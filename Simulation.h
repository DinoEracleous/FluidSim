#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

const unsigned int NUM_PARTICLES = 100;
const glm::vec2 GRID_DIMENSIONS = glm::vec2(50,40);
const float SPACING = 1.1f;
const int GRAVITY = -9.81;
const int NUM_ITERS = 2; //number of iterations to repeat pushApart


struct Particle{
    glm::vec2 position;
    glm::vec2 velocity;
};

enum cellType {WATER, AIR, SOLID};

struct fluidCell{
    glm::vec2 prevVelocity;
    glm::vec2 velocity;
    cellType type;
};

class Simulation {
public:
    glm::ivec2 gridDimensions = GRID_DIMENSIONS;
    float gravity = GRAVITY;
    std::vector<Particle> particles;
    Simulation() : particles(NUM_PARTICLES), grid(gridDimensions.x*gridDimensions.y + 1,0), particleIDs(NUM_PARTICLES,0), 
                   fluidGrid(gridDimensions.x*gridDimensions.y)
    {
        //set particles initial conditions
        for(int i{};i<particles.size();i++){
            particles.at(i).position = glm::vec2((i%(gridDimensions.x/2))+spacing+particleRadius,(2*i/gridDimensions.x)+spacing+particleRadius);
            particles.at(i).velocity = glm::vec2(5.0f,15.0f);
        }
        //set wall cells to be solid else they are set to air.
        for(int i{};i<gridDimensions.x;i++){
            for (int j{};j<gridDimensions.y;j++){
                int index = gridCoordIndex({i,j});
                if(i==0 || j==0 || i == gridDimensions.x-1 || j==gridDimensions.y-1){
                    fluidGrid.at(index).type = SOLID;
                }
                fluidGrid.at(index).type = AIR;
            }
        }
    }
    void simulate(float dt){
        integrate(dt);
        pushApart(NUM_ITERS);
        handleObstacles();
        transferVelocties(true);
    }

private:
    float particleRadius = 0.5f;
    float spacing = SPACING; //size of one grid cell
    std::vector<int> grid; //collision grid column by column for spatial hash.
    std::vector<int> particleIDs; //indices of particles arranged by cell.
    std::vector<fluidCell> fluidGrid; // each cell is air, water or solid and has velocities moving into it.
    
    
    //get the coordinate of the grid cell in which the particle is currently located
    glm::ivec2 getGridCoords(glm::vec2 pos){
        glm::ivec2 coords {(int)std::floor(pos.x/spacing),(int)std::floor(pos.y/spacing)};
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

    void pushApart(int numIters){
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
                
    //keep particles out of walls
    void handleObstacles(){
        float leftWall {spacing}, rightWall {spacing*gridDimensions.x-spacing}, lowerWall {spacing}, upperWall{spacing * gridDimensions.y-spacing};
        for(int i{};i<NUM_PARTICLES;i++){
            Particle &p = particles.at(i);
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

    void transferVelocties(bool toGrid){
        if(toGrid){
            for(int i{};i<fluidGrid.size();i++){
                fluidGrid.at(i).prevVelocity = fluidGrid.at(i).velocity; //make a copy of velocities for later
                fluidGrid.at(i).velocity = {0.0f,0.0f};
                fluidGrid.at(i).type = (fluidGrid.at(i).type!= SOLID?AIR:SOLID);
            }
            for(int i{};i<NUM_PARTICLES;i++){
                int index = gridCoordIndex(getGridCoords(particles.at(i).position));           
                fluidGrid.at(index).type = WATER;
            }
        }
    }
};

#endif
    
