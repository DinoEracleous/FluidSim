#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "stb_image.h"
#include "camera.h"
#include "Simulation.h"

#include <iostream>
#include <cmath>
#include <string>
#include <vector>

using std::sin;

GLFWwindow* setupWindow();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
unsigned int loadTexture(const std::string path);
void drawBalls(std::vector<Particle> particles);
void drawBalls(std::vector<glm::vec2> positions,float radius, glm::vec3 color);
void drawLine(glm::vec2 p1 , glm::vec2 p2);

// settings
unsigned int SCREEN_WIDTH = 1200;
unsigned int SCREEN_HEIGHT = 900;
float ASPECT_RATIO = 12.0f/9.0f;

unsigned int textureCount {0};

Camera camera;
Shader ballShader;
Shader lineShader;

Simulation sim;

int main()
{
    //Window setup
    GLFWwindow* window = setupWindow();
    if (window==NULL){
        glfwTerminate();
        std::cout << "ERROR Failed to initialise window" << std::endl;
        return -1;
    }

    //Setup GLAD: Get locations of all opengl functions and load them
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "ERROR Failed to initialize GLAD" << std::endl;
        return -1;
    } 

    //opengl state configuration
    glEnable(GL_DEPTH_TEST);
    
    //===========Vertex data=============

    //quad for drawing circles
    float quadVertices[] = {
       -0.5,-0.5,0.0,
        0.5,-0.5,0.0,
        0.5, 0.5,0.0,
       -0.5, 0.5,0.0 
    };
    
    //quad indices
    unsigned int quadIndices[] = {
        0,1,2,
        2,0,3
    };

    //line vertices
    float lineVertices[] = {
        0.0,0.0,0.0,
        1.0,1.0,0.0
    };

    //============Buffers================
    //QUAD
    unsigned int quadVBO, quadVAO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO); 
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,quadEBO);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(quadIndices),quadIndices,GL_STATIC_DRAW);

    //position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    //LINE
    unsigned int lineVBO, lineVAO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 


    //=============SHADERS===============
    ballShader.genShaderProgram("vertex.vert", "fragment.frag");
    lineShader.genShaderProgram("vertex.vert", "line.frag");

    //===========Transforms==============
   
    //BALLS
    ballShader.use();
    glm::mat4 model = glm::mat4(1.0f); 
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    //===========Simulation==============
    float gridSpacing = SPACING;
    int gridx = GRID_DIMENSIONS.x;
    int gridy = GRID_DIMENSIONS.y;

    //CAMERA
    camera.Position = glm::vec3(gridx/2, gridy/2, 250.0f);
    float lastTime {(float)glfwGetTime()};

    //Render loop
    while(!glfwWindowShouldClose(window))
    {
        float timeNow {(float)glfwGetTime()};
        float deltaTime {timeNow-lastTime};
        lastTime = timeNow;

        processInput(window, deltaTime);

        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        ballShader.use();
        projection = glm::perspective(glm::radians(camera.fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, camera.near, camera.far);
        ballShader.setMat4("projection", projection);
        view = camera.GetViewMatrix();
        ballShader.setMat4("view",view);
        
        sim.simulate(deltaTime);

        glBindVertexArray(quadVAO);
        drawBalls(sim.particles);
        drawBalls({sim.mouseObstacle.position},sim.mouseObstacle.radius, sim.mouseObstacle.color);

        //draw lines for boundaries 
        lineShader.use();
        lineShader.setMat4("projection", projection);
        lineShader.setMat4("view",view);
        glBindVertexArray(lineVAO);
        drawLine({gridSpacing,gridSpacing},{gridx*gridSpacing-gridSpacing,gridSpacing}); //floor
        drawLine({gridSpacing,gridSpacing},{gridSpacing,gridy*gridSpacing-gridSpacing}); //left wall
        drawLine({gridSpacing,gridy*gridSpacing-gridSpacing},{gridx*gridSpacing-gridSpacing,gridy*gridSpacing-gridSpacing}); //ceiling
        drawLine({gridx*gridSpacing-gridSpacing,gridSpacing},{gridx*gridSpacing-gridSpacing,gridy*gridSpacing-gridSpacing}); //right wall
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    //clean up
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1,&quadEBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    ballShader.deleteProgram();
    lineShader.deleteProgram();
      
    glfwTerminate();
    return 0;
}

void drawBalls(std::vector<Particle> particles){
    for(auto const &particle: particles){
        glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(particle.position,0.0f));
        ballShader.setMat4("model",model);
        ballShader.setVec3("color", particle.color);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    }
}

void drawBalls(std::vector<glm::vec2> positions, float radius, glm::vec3 color){
    for(auto const &pos: positions){
        glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(pos,0.1f));
        model = glm::scale(model,glm::vec3(2*radius));
        ballShader.setMat4("model",model);
        ballShader.setVec3("color", color);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    }
}

void drawLine(glm::vec2 p1, glm::vec2 p2){ //point1 to point2
    glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(p1,0.01f));
    model = glm::scale(model,glm::vec3(p2-p1,0.0f));
    lineShader.setMat4 ("model", model);
    glDrawArrays(GL_LINES,0,2);
}

GLFWwindow* setupWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Super Awesome Fluid Sim", NULL, NULL);
    if (window != NULL){
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
    }
    
    return window;
}

void processInput(GLFWwindow *window, float deltaTime)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS)
        camera.ProcessKeys(UP,deltaTime);
    if(glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS)
        camera.ProcessKeys(LEFT,deltaTime);
    if(glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS)
        camera.ProcessKeys(RIGHT,deltaTime);
    if(glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS)
        camera.ProcessKeys(DOWN,deltaTime);
    if(glfwGetKey(window, GLFW_KEY_SPACE)==GLFW_PRESS)
        camera.ProcessKeys(FORWARD,deltaTime);
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS)
        camera.ProcessKeys(BACK,deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){

    float scale = 2.0f*camera.Position.z * std::tan(glm::radians(camera.fov/2))/SCREEN_HEIGHT;
    sim.mouseObstacle.position = {scale*(xpos-(SCREEN_WIDTH/2)) + camera.Position.x,scale*(-ypos +(SCREEN_HEIGHT/2)) + camera.Position.y};
    //std::cout << scale*(xpos-(SCREEN_WIDTH/2)) + camera.Position.x <<", "<< scale*(-ypos +(SCREEN_HEIGHT/2)) + camera.Position.y << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    // SCREEN_HEIGHT = height;
    // SCREEN_WIDTH = height * ASPECT_RATIO;
    // glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glViewport(0,0,width,height);
    SCREEN_HEIGHT = height;
    SCREEN_WIDTH = width;
}

unsigned int loadTexture(const std::string path){
    unsigned int texture;
    GLenum format {GL_RGB};
    if(path.substr(path.size()-3,3)=="png")
        format = GL_RGBA;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + textureCount);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0); 
    stbi_set_flip_vertically_on_load(true);
    if (data){
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }else{
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    textureCount++;
    return texture;
}
