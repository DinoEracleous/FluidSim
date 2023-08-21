#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

const unsigned int NUM_PARTICLES = 5000;
const glm::vec2 GRID_DIMENSIONS = glm::vec2(200,80);
const float SPACING = 1.1f;
const int GRAVITY = -9.81f;
const int NUM_ITERS = 3; //number of iterations to repeat pushApart
const float FLIP_PIC_RATIO = 0.9f;
const float OVERRELAX = 1.9f;
const float COMPRESSION_FACTOR = 2.0f;
const float MOUSE_OBSTACLE_RADIUS = 15.0f;
const float TIME_SCALE = 2.0f;


struct Particle{
    glm::vec2 position;
    glm::vec2 velocity;
};

enum cellType {WATER, AIR, SOLID};

struct fluidCell{
    glm::vec2 prevVelocity;
    glm::vec2 velocity;
    glm::vec2 weights;
    float density;
    cellType type {AIR};
};

struct ballObstacle{
    glm::vec2 position;
    glm::vec2 velocity;
    float radius;
    glm::vec2 prevPos;
};

class Simulation {
public:
    glm::ivec2 gridDimensions = GRID_DIMENSIONS;
    float gravity = GRAVITY;
    std::vector<Particle> particles;
    ballObstacle mouseObstacle{{50.0f,50.0f},{0.0f,0.0f},MOUSE_OBSTACLE_RADIUS,{50.0f,50.0f}}; //mouse controls a ball where particles will be pushed away.

    Simulation() : particles(NUM_PARTICLES), grid(gridDimensions.x*gridDimensions.y + 1,0), particleIDs(NUM_PARTICLES,0), 
                   fluidGrid(gridDimensions.x*gridDimensions.y)
    {
        //set particles initial conditions
        for(int i{};i<particles.size();i++){
            particles.at(i).position = glm::vec2((i%(gridDimensions.x/2))+spacing+particleRadius,(2*i/gridDimensions.x)+spacing+particleRadius);
            particles.at(i).velocity = glm::vec2(10.0f,10.0f);
        }
        //set wall cells to be solid else they are set to air.
        for(int i{};i<gridDimensions.x;i++){
            for (int j{};j<gridDimensions.y;j++){
                int index = gridCoordIndex({i,j});
                if(i==0 || j==0 || i == gridDimensions.x-1 || j==gridDimensions.y-1){
                    fluidGrid.at(index).type = SOLID;
                } else {
                    fluidGrid.at(index).type = AIR;

                }
            }
        }

    }
    void simulate(float dt){
        //integrate(2*dt); 
        integrate(TIME_SCALE*dt);
        pushApart();
        handleObstacles(TIME_SCALE*dt);
        transferVelocities(true,FLIP_PIC_RATIO);
        computeDensities();
        makeIncompressible();
        transferVelocities(false,FLIP_PIC_RATIO);
    }

private:
    float particleRadius = 0.5f;
    float spacing = SPACING; //size of one grid cell
    std::vector<int> grid; //collision grid column by column for spatial hash.
    std::vector<int> particleIDs; //indices of particles arranged by cell.
    std::vector<fluidCell> fluidGrid; // each cell is air, water or solid and has velocities moving into it.
    int numIters = NUM_ITERS;
    float restDensity;
    
    //get the coordinate of the grid cell in which the particle is currently located
    glm::ivec2 getGridCoords(glm::vec2 pos){
        glm::ivec2 coords {(int)std::floor(pos.x/spacing),(int)std::floor(pos.y/spacing)};
        coords = glm::clamp(coords,{0,0},{gridDimensions.x-1,gridDimensions.y-1});
        return coords;
    }

    //get the 1D index for a given 2D grid cell coordinate
    int gridCoordIndex(glm::ivec2 coord){
        int index {static_cast<int>(gridDimensions.y * coord.x + coord.y)};
        return index;
    }

    //semi implicit euler integration to calculate particle positions under gravity.
    void integrate(float dt){
        for(auto &particle:particles){
            particle.velocity += glm::vec2(0.0f, dt*gravity);
            particle.position += dt*particle.velocity;
        }
    }

    //push particles out of each other
    void pushApart(){
        //FILL SPATIAL HASH GRID
        //clear grid
        grid = std::vector<int>(gridDimensions.x*gridDimensions.y + 1,0);
        //count number of particles in each cell
        for(int i{};i<NUM_PARTICLES;i++){
            int gridIndex = gridCoordIndex(getGridCoords(particles.at(i).position));
            grid.at(gridIndex)++;
        }
        //insert running total particle counts
        int current {};
        for(int i{};i<grid.size()-1;i++){
            current += grid.at(i);
            grid.at(i) = current;
        }

        grid.at(grid.size()-1) = NUM_PARTICLES; //guard
        
        //fill particleIDs
        for(int i{};i<NUM_PARTICLES;i++){
            int gridIndex = gridCoordIndex(getGridCoords(particles.at(i).position));
            particleIDs.at(--grid.at(gridIndex)) = i; 
        }

        //PUSH PARTICLES APART
        for (int iter{};iter<numIters;iter++){
            for(int i{};i<NUM_PARTICLES;i++){
                Particle &p {particles.at(i)};
                glm::ivec2 gridCoords = getGridCoords(p.position);
                int xStart {std::max(gridCoords.x-1,1)}, xEnd {std::min(gridCoords.x+1,gridDimensions.x-1)};
                int yStart {std::max(gridCoords.y-1,1)}, yEnd {std::min(gridCoords.y+1,gridDimensions.y-1)};
                for(int xi{xStart};xi<=xEnd;xi++){
                    for(int yi{yStart};yi<=yEnd;yi++){
                        int index = gridCoordIndex({xi,yi});
                        for(int pi{grid[index]};pi<grid[index+1];pi++){
                            glm::vec2 &p2 {particles.at(particleIDs[pi]).position};
                            if(pi==i || glm::dot(p2-p.position,p2-p.position)>=4.0f*particleRadius*particleRadius) continue;
                            float distance = glm::length(p2-p.position);
                            
                            if(distance != 0.0f){
                                glm::vec2 normed = (p2-p.position)*(1.0f/distance);
                                glm::vec2 movep {(normed*(particleRadius-(distance/2.0f)))};
                                p.position -= movep;
                                p2 += movep;
                            }
                        }
                    }
                }
            }
        }
    }
                
    //push particles out of walls
    void handleObstacles(float dt){
        //update mouse obstacle velocity
        mouseObstacle.velocity = (mouseObstacle.position- mouseObstacle.prevPos)/(TIME_SCALE*dt);
        mouseObstacle.prevPos = mouseObstacle.position;

        float leftWall {spacing}, rightWall {spacing*gridDimensions.x-spacing}, lowerWall {spacing}, upperWall{spacing * gridDimensions.y-spacing};
        for(int i{};i<NUM_PARTICLES;i++){
            Particle &p = particles.at(i);
            //mouse obstacle
            float edgeDist2 {(mouseObstacle.radius+particleRadius)*(mouseObstacle.radius+particleRadius)};
            float dist2 {glm::dot(p.position-mouseObstacle.position,p.position-mouseObstacle.position)};
            if(dist2 < edgeDist2){
                p.velocity += 0.3f * mouseObstacle.velocity;
            }

            //walls
            if(p.position.x < leftWall+particleRadius){
                p.position.x = leftWall + particleRadius;
                p.velocity.x = 0.0f;
            }
            if(p.position.x > rightWall-particleRadius){
                p.position.x = rightWall-particleRadius;
                p.velocity.x = 0.0f;
            }
            if(p.position.y < lowerWall+particleRadius){
                p.position.y = lowerWall+particleRadius;
                p.velocity.y = 0.0f;
            }
            if(p.position.y > upperWall-particleRadius){
                p.position.y = upperWall-particleRadius;
                p.velocity.y = 0.0f;
            }
        }
    }

    //transfer particle velocities to and from the fluidGrid
    void transferVelocities(bool toGrid, float flipPicRatio){
        if(toGrid){
            //clear cell velocities and weights
            for(int i{};i<fluidGrid.size();i++){
                fluidGrid.at(i).velocity = {0.0f,0.0f};
                fluidGrid.at(i).weights = {0.0f,0.0f};
                fluidGrid.at(i).type = (fluidGrid.at(i).type!= SOLID?AIR:SOLID);
            }
            //set cells to water if they contain any particles.
            for(int i{};i<NUM_PARTICLES;i++){
                int index = gridCoordIndex(getGridCoords(particles.at(i).position));           
                fluidGrid.at(index).type = WATER;
            }
        }

        for(int component{};component<2;component++){ //horizontal component then vertical component
            for(int i{};i<NUM_PARTICLES;i++){ //calculate weights and transfer velocities
                //calculate weights for horizontal grid velocities
                glm::vec2 pos {particles.at(i).position.x,particles.at(i).position.y}; 
                pos -= glm::vec2({component*spacing/2.0f,(1-component)*spacing/2.0f}); //shift particle for staggered grid
                pos.x = glm::clamp(pos.x,spacing,spacing*(gridDimensions.x-1)); //keep pos in bounds
                pos.y = glm::clamp(pos.y,spacing,spacing*(gridDimensions.y-1));
                glm::ivec2 q0{getGridCoords(pos)};//coords of 4 surrounding cells
                glm::ivec2 q1{std::min(q0.x+1,gridDimensions.x-2),q0.y};
                glm::ivec2 q2{q1.x,std::min(q1.y+1,gridDimensions.y-2)};
                glm::ivec2 q3{q0.x,std::min(q0.y+1,gridDimensions.y-2)};
                int i0{gridCoordIndex(q0)},i1{gridCoordIndex(q1)},i2{gridCoordIndex(q2)},i3{gridCoordIndex(q3)}; //fluidGrid indices for cells
                float dx{pos.x-q0.x*spacing}, dy{pos.y-q0.y*spacing}; //here the repeated parts of the bilinear interp values are calculated
                float sx {dx/spacing}, sy{dy/spacing};
                float tx {1-sx}, ty {1-sy};
                float w0{tx*ty}, w1{sx*ty}, w2{sx*sy} ,w3{tx*sy}; //weights


                if(toGrid){ //sum weighted velocities and weights for each cell.
                    fluidGrid.at(i0).velocity[component] += w0*particles.at(i).velocity[component];
                    fluidGrid.at(i1).velocity[component] += w1*particles.at(i).velocity[component];
                    fluidGrid.at(i2).velocity[component] += w2*particles.at(i).velocity[component];
                    fluidGrid.at(i3).velocity[component] += w3*particles.at(i).velocity[component];
                    fluidGrid.at(i0).weights[component] += w0;
                    fluidGrid.at(i1).weights[component] += w1;
                    fluidGrid.at(i2).weights[component] += w2;
                    fluidGrid.at(i3).weights[component] += w3;
                } else { //handle transfer from grid to particles
                    //ensure we do not consider velocities between two air cells
                    int adjacentOffset = (component)?1:gridDimensions.y;
                    bool isValid0 {fluidGrid.at(i0).type != AIR || fluidGrid.at(i0-adjacentOffset).type != AIR};
                    bool isValid1 {fluidGrid.at(i1).type != AIR || fluidGrid.at(i1-adjacentOffset).type != AIR};
                    bool isValid2 {fluidGrid.at(i2).type != AIR || fluidGrid.at(i2-adjacentOffset).type != AIR};
                    bool isValid3 {fluidGrid.at(i3).type != AIR || fluidGrid.at(i3-adjacentOffset).type != AIR};
                    
                    float w = isValid0*w0 + isValid1*w1 + isValid2*w2 + isValid3*w3;
                    if(w > 0.0f){ //average out grid velocities
                        float pic = (isValid0*w0*fluidGrid.at(i0).velocity[component] +
                                    isValid1*w1*fluidGrid.at(i1).velocity[component] +
                                    isValid2*w2*fluidGrid.at(i2).velocity[component] +
                                    isValid3*w3*fluidGrid.at(i3).velocity[component])/w;
                        float flipDelta = (isValid0*w0*(fluidGrid.at(i0).velocity[component]-fluidGrid.at(i0).prevVelocity[component]) +
                                    isValid1*w1*(fluidGrid.at(i1).velocity[component]-fluidGrid.at(i1).prevVelocity[component]) +
                                    isValid2*w2*(fluidGrid.at(i2).velocity[component]-fluidGrid.at(i2).prevVelocity[component]) +
                                    isValid3*w3*(fluidGrid.at(i3).velocity[component]-fluidGrid.at(i3).prevVelocity[component]))/w;
                        float flip = flipDelta + particles.at(i).velocity[component];
                        particles.at(i).velocity[component] = flipPicRatio*flip + (1.0f-flipPicRatio)*pic; //transfer to particles
                    }

                }

            }
        }
        if(toGrid){
            for(int i{};i<fluidGrid.size();i++){
                if(fluidGrid.at(i).weights.x > 0.0f)
                    fluidGrid.at(i).velocity.x /= fluidGrid.at(i).weights.x;
                if(fluidGrid.at(i).weights.y > 0.0f)
                    fluidGrid.at(i).velocity.y /= fluidGrid.at(i).weights.y; 
            }
        }
    }

    void makeIncompressible(){
        for(int i{};i<fluidGrid.size();i++){
            fluidGrid.at(i).prevVelocity = fluidGrid.at(i).velocity; //make a copy of velocities for later
        }
        for (int iter{};iter<numIters;iter++){
            for(int i{1};i<gridDimensions.x-1;i++){
                for(int j{1};j<gridDimensions.y-1;j++){
                    if(fluidGrid.at(gridCoordIndex({i,j})).type != WATER) continue;
                    float div {fluidGrid.at(gridCoordIndex({i+1,j})).velocity.x -
                               fluidGrid.at(gridCoordIndex({i,j})).velocity.x +
                               fluidGrid.at(gridCoordIndex({i,j+1})).velocity.y -
                               fluidGrid.at(gridCoordIndex({i,j})).velocity.y};
                    int sLeft {fluidGrid.at(gridCoordIndex({i-1,j})).type!=SOLID?1:0};
                    int sRight {fluidGrid.at(gridCoordIndex({i+1,j})).type!=SOLID?1:0};
                    int sBottom {fluidGrid.at(gridCoordIndex({i,j-1})).type!=SOLID?1:0};
                    int sTop {fluidGrid.at(gridCoordIndex({i,j+1})).type!=SOLID?1:0};
                    div *= OVERRELAX;
                    //adjust for drift
                    float compression = fluidGrid.at(gridCoordIndex({i,j})).density - restDensity;
                    if (compression>0.0f) 
                        div -= COMPRESSION_FACTOR*compression; 
                    float s = sLeft + sRight + sBottom + sTop;
                    if (s==0) continue;
                    div /= s;
                    fluidGrid.at(gridCoordIndex({i,j})).velocity.x += div*sLeft;
                    fluidGrid.at(gridCoordIndex({i+1,j})).velocity.x -= div*sRight;
                    fluidGrid.at(gridCoordIndex({i,j})).velocity.y += div*sBottom;
                    fluidGrid.at(gridCoordIndex({i,j+1})).velocity.y -= div*sTop;
                }
            }
        }

    }

    void computeDensities(){
        //clear densities;
        for(int i{};i<fluidGrid.size();i++){
            fluidGrid.at(i).density = 0.0f;
        }

        //calculate weights
        for(int i{};i<NUM_PARTICLES;i++){
            glm::vec2 pos {particles.at(i).position.x,particles.at(i).position.y}; 
            pos -= glm::vec2({spacing/2.0f,spacing/2.0f}); //shift both coordinates so we calulate density at the center of each cell
            pos.x = glm::clamp(pos.x,spacing,spacing*(gridDimensions.x-1)); //keep pos in bounds
            pos.y = glm::clamp(pos.y,spacing,spacing*(gridDimensions.y-1));
            //coords of 4 surrounding cells
            glm::ivec2 q0{getGridCoords(pos)};
            glm::ivec2 q1{std::min(q0.x+1,gridDimensions.x-2),q0.y};
            glm::ivec2 q2{q1.x,std::min(q1.y+1,gridDimensions.y-2)};
            glm::ivec2 q3{q0.x,std::min(q0.y+1,gridDimensions.y-2)};
            int i0{gridCoordIndex(q0)},i1{gridCoordIndex(q1)},i2{gridCoordIndex(q2)},i3{gridCoordIndex(q3)}; //fluidGrid indices for cells
            float dx{pos.x-q0.x*spacing}, dy{pos.y-q0.y*spacing}; //here the repeated parts of the bilinear interp values are calculated
            float sx {dx/spacing}, sy{dy/spacing};
            float tx {1-sx}, ty {1-sy};
            float w0{tx*ty}, w1{sx*ty}, w2{sx*sy} ,w3{tx*sy}; //weights

            //sum weights to get densities
            fluidGrid.at(i0).density += w0;
            fluidGrid.at(i1).density += w1;
            fluidGrid.at(i2).density += w2;
            fluidGrid.at(i3).density += w3;
        }
        
        //On first execution we set the initial density
        if (restDensity==0.0f){
            float densitySum{};
            int numWater {};
            for(int i{};i<fluidGrid.size();i++){
                if(fluidGrid.at(i).type == WATER){
                    densitySum += fluidGrid.at(i).density;
                    numWater++; //count number of water cells
                }
            }
            if(numWater!=0.0f) restDensity = densitySum/numWater;
        }


    }

};

#endif
    
