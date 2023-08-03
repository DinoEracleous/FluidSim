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

struct Particle{
    glm::vec2 position;
    glm::vec2 velocity;
};

class Simulation {
public:
    glm::ivec2 gridDimensions = GRID_DIMENSIONS;
    float gravity = GRAVITY;
    std::vector<Particle> particles;
    Simulation() : particles(NUM_PARTICLES), grid(gridDimensions.x*gridDimensions.y + 1,0) {
        //set particles initial conditions
        for(int i{};i<particles.size();i++){
            particles.at(i).position = glm::vec2((i%(gridDimensions.x/2))+0.5,(2*i/gridDimensions.x)+0.5);
            particles.at(i).velocity = glm::vec2(1.0f,1.0f);
        }
    }
    void simulate(){
        integrate(1.0f/300);
        handleObstacles();
    }

private:
    int particleRadius = 0.5;
    float spacing = 1.1f; //size of one grid cell
    std::vector<int> grid;
    
    //get the coordinate of the grid cell in which the particle is currently located
    glm::ivec2 getGridCoords(glm::vec2 pos){
        return glm::ivec2(std::floor(pos.x/spacing),std::floor(pos.y/spacing));
    }

    //get the 1D index for a given 2D grid cell coordinate
    int gridCoordIndex(glm::ivec2 coord){
        return static_cast<int>(gridDimensions.x * coord.x + coord.y);
    }

    //semi implicit euler integration to calculate particle positions under gravity.
    void integrate(float dt){
        for(auto &particle:particles){
            particle.velocity += glm::vec2(0.0f, dt*gravity);
            particle.position += particle.velocity;
        }
    }

    //keep particles out of walls
    void handleObstacles(){
        float leftWall {0}, rightWall {spacing*gridDimensions.x}, lowerWall {0}, upperWall{spacing * gridDimensions.y};
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