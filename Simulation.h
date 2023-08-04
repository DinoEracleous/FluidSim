#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>

const unsigned int NUM_PARTICLES = 1000;
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
            particles.at(i).position = glm::vec2((i%(gridDimensions.x/2))+0.5,(2*i/gridDimensions.x)+0.5);
            particles.at(i).velocity = glm::vec2(1.0f,1.0f);
        }
    }
    void simulate(float dt){
        integrate(dt);
        pushApart(NUM_ITERS);
        handleObstacles();
    }

private:
    int particleRadius = 0.5;
    float spacing = 1.1f; //size of one grid cell
    std::vector<int> grid;
    std::vector<int> particleIDs;
    
    
    //get the coordinate of the grid cell in which the particle is currently located
    glm::ivec2 getGridCoords(glm::vec2 pos){
        return glm::ivec2(std::floor(pos.x/spacing),std::floor(pos.y/spacing));
    }

    //get the 1D index for a given 2D grid cell coordinate
    int gridCoordIndex(glm::ivec2 coord){
        return static_cast<int>(gridDimensions.y * coord.x + coord.y);
    }

    //semi implicit euler integration to calculate particle positions under gravity.
    void integrate(float dt){
        for(auto &particle:particles){
            particle.velocity += glm::vec2(0.0f, dt*gravity);
            particle.position += particle.velocity;
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
            current += grid[i];
            grid.at(i) = current;
        }
        grid[grid.size()-1] = NUM_PARTICLES; //guard
        //fill particleIDs
        for(int i{};i<NUM_PARTICLES;i++){
            int gridIndex = gridCoordIndex(getGridCoords(particles.at(i).position));
            particleIDs[--grid.at(gridIndex)] = i; 
        }

        //PUSH PARTICLES APART
        for (int i{};i<numIters;i++){

        }


    }

    //keep particles out of walls
    void handleObstacles(){
        float leftWall {spacing}, rightWall {spacing*gridDimensions.x-spacing}, lowerWall {spacing}, upperWall{spacing * gridDimensions.y-spacing};
        for(int i{};i<NUM_PARTICLES;i++){
            Particle &p = particles.at(i);
            if(p.position.x < leftWall+particleRadius){
                p.position.x = 0.0f + particleRadius;
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