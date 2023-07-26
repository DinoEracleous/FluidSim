#ifndef _CAMERA_H
#define _CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum Camera_Movement {FORWARD, BACK, LEFT, RIGHT, UP, DOWN};

// Defaults
const float SPEED       =  25.0f;
const float SENS        =  0.1f;

class Camera
{
public:
    // camera Attributes
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right = glm::vec3(1.0f,0.0f,0.0f);

    float MovementSpeed = SPEED;
    float MouseSensitivity = SENS;

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeys(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == UP)
            Position -= Up * velocity;
        if (direction == DOWN)
            Position += Up * velocity;
        if (direction == LEFT)
            Position += Right * velocity;
        if (direction == RIGHT)
            Position -= Right * velocity;
        if (direction == FORWARD)
            Position -= Front * velocity;
        if (direction == BACK)
            Position += Front * velocity;
    }
};
#endif