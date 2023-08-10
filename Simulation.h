#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>
#include <iostream>

const unsigned int NUM_PARTICLES = 100;
const glm::vec2 GRID_DIMENSIONS = glm::vec2(50,40);
const int GRAVITY = -9.81;
const int NUM_ITERS = 2; //number of iterations to repeat pushApart

struct Particle{
    glm::vec2 position;
    glm::vec2 velocity;
};

class Simulation {
public:
    glm::ivec2 gridDimensions = GRID_DIMENSIONS;
    float gravity = GRAVITY;
    std::vector<Particle> particles;
    Simulation() : particles(NUM_PARTICLES), grid(gridDimensions.x*gridDimensions.y + 1,0), particleIDs(NUM_PARTICLES,0) {
        //set particles initial conditions
        for(int i{};i<particles.size();i++){
            particles.at(i).position = glm::vec2((i%(gridDimensions.x/2))+spacing+particleRadius,(2*i/gridDimensions.x)+spacing+particleRadius);
            particles.at(i).velocity = glm::vec2(20.0f,10.0f);
        }
    }
    void simulate(float dt){
        integrate(dt);
        pushApart(NUM_ITERS);
        handleObstacles();
    }

private:
    float particleRadius = 0.5f;
    float spacing = 1.1f; //size of one grid cell
    std::vector<int> grid; //collision grid column by column
    std::vector<int> particleIDs; //indices of particles arranged by cell.
    
    
    //get the coordinate of the grid cell in which the particle is currently located
    glm::ivec2 getGridCoords(glm::vec2 pos){
        glm::ivec2 coords {std::floor(pos.x/spacing),std::floor(pos.y/spacing)};
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
            for(int i{};i<grid.size()-1;i++){
                int particlesInCell = grid[i+1]-grid[i];
                for(int p1{grid[i]};p1<=grid[i]+particlesInCell-2;p1++){
                    for(int p2{p1+1};p2<=grid[i]+particlesInCell-1;p2++){
                        glm::vec2 &point1 {particles.at(particleIDs[p1]).position};
                        glm::vec2 &point2 {particles.at(particleIDs[p2]).position};
                        if(glm::dot(point2-point1,point2-point1)>4*particleRadius*particleRadius) continue;
                        float distance = glm::length(point2-point1);
                        
                        if(distance != 0.0f){
                            glm::vec2 normed = (point2-point1)*(1.0f/distance);
                            glm::vec2 movep {(normed*(particleRadius-distance/2.0f))};
                            point1 -= movep;
                            point2 += movep;
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
};

#endif
    
