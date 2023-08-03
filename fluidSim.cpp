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
unsigned int loadTexture(const std::string path);
void drawBalls(std::vector<Particle> particles);

// settings
unsigned int SCREEN_WIDTH = 1200;
unsigned int SCREEN_HEIGHT = 900;
float ASPECT_RATIO = 12.0f/9;

unsigned int textureCount {0};

Camera camera;
Shader shader;

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
    float vertices[] = {
       -0.5,-0.5,0.0,
        0.5,-0.5,0.0,
        0.5, 0.5,0.0,
       -0.5, 0.5,0.0 
    };

    unsigned int indices[] = {
        0,1,2,
        2,0,3
    };

    
    //============Buffers================
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

    //position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);//tells opengl the format of the vertex buffer.
    glEnableVertexAttribArray(0); 

    //=============SHADERS===============
    shader.genShaderProgram("vertex.vert", "fragment.frag");
    shader.use();

    //===========Transforms==============
   
    glm::mat4 model = glm::mat4(1.0f); 
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 200.0f);
    shader.setMat4("projection", projection);
    
    camera.Position = glm::vec3(12.0f,10.0f,100.0f);

    float lastTime {(float)glfwGetTime()};
    //===========Simulation==============
    Simulation sim;


    //Render loop
    while(!glfwWindowShouldClose(window))
    {
        float timeNow {(float)glfwGetTime()};
        float deltaTime {timeNow-lastTime};
        lastTime = timeNow;

        processInput(window, deltaTime);

        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = camera.GetViewMatrix();
        shader.setMat4("view",view);

        sim.simulate();
        drawBalls(sim.particles);

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shader.deleteProgram();
      
    glfwTerminate();
    return 0;
}

void drawBalls(std::vector<Particle> particles){
    for(auto const &particle: particles){
        glm::mat4 model = glm::translate(glm::mat4(1.0f),glm::vec3(particle.position,0.0f));
        shader.setMat4("model",model);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    }
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
        SCREEN_HEIGHT = height;
        SCREEN_WIDTH = height * ASPECT_RATIO;
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
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
