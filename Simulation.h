#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>

struct Particle{
    glm::vec2 position;
    glm::vec2 velocity;
};

class Simulation {
public:
    int numParticles = 1000;
    glm::ivec2 gridDimensions = glm::ivec2(50,40);
    float gravity = -9.81;
    Simulation() : grid(gridDimensions.x*gridDimensions.y + 1,0) {}
    void simulate(){

    }

private:
    int particleRadius = 0.5;
    float spacing = 1.1f; //size of one grid cell
    std::vector<int> grid;
    std::vector<Particle> particles;

    glm::ivec2 getGridCoords(glm::vec2 pos){
        return glm::ivec2(std::floor(pos.x/spacing),std::floor(pos.y/spacing));
    }
    int gridCoordIndex(glm::ivec2 coord){
        return static_cast<int>(gridDimensions.x * coord.x + coord.y);
    }

    void integrate(float dt){
        for(auto &particle:particles){
            particle.velocity += glm::vec2(0.0f, dt*gravity);
            particle.position += particle.velocity;
        }
    }

    
};

#endif