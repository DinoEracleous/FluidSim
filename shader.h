#ifndef _SHADER_H
#define _SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    unsigned int genShaderProgram(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexStr = readSource(vertexPath);
        std::string fragmentStr = readSource(fragmentPath);

        const char* vertexSource = vertexStr.c_str();
        const char * fragmentSource = fragmentStr.c_str();
        
        unsigned int vertexID, fragmentID;

        // vertex shader
        vertexID = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexID, 1, &vertexSource, NULL);
        glCompileShader(vertexID);
        checkCompileErrors(vertexID, "VERTEX");

        // fragment Shader
        fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentID, 1, &fragmentSource, NULL);
        glCompileShader(fragmentID);
        checkCompileErrors(fragmentID, "FRAGMENT");

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertexID);
        glAttachShader(ID, fragmentID);
        glLinkProgram(ID);
        checkLinkingErrors();
        
        //linked shaders can be deleted
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);
        return ID;
    }

    void use() { 
        glUseProgram(ID); 
    }

    void deleteProgram(){
        glDeleteProgram(ID);
    }

    // Methods for setting uniforms
    
    void setBool(const std::string &name, bool value) const{         
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    
    void setInt(const std::string &name, int value) const{ 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }

    void setFloat(const std::string &name, float value) const{ 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const{ 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }

    void setVec2(const std::string &name, float x, float y) const{ 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }

    void setVec3(const std::string &name, const glm::vec3 &value) const{ 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }

    void setVec3(const std::string &name, float x, float y, float z) const{ 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const{ 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }

    void setVec4(const std::string &name, float x, float y, float z, float w) const{ 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const{
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const{
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const{
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:

    std::string readSource(const char* path){
        std::ifstream sourceFile;
        std::string sourceString {};
        sourceFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            sourceFile.open(path);
            std::stringstream shaderStream;
            shaderStream << sourceFile.rdbuf();
            sourceFile.close();
            sourceString = shaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR SHADER FILE \'"<< path <<"\' COULD NOT BE READ" << std::endl;
            std::cout << e.what() << std::endl;
            std::cout << "---------" << std::endl;
        }
        return sourceString;
    }
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR " << type << " SHADER FAILED TO COMPILE \n" << infoLog << std::endl;
            std::cout << "---------" << std::endl;
        }
    }
        
    void checkLinkingErrors(){
        int success;
        char infoLog[1024];

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 1024, NULL, infoLog);
            std::cout << "ERROR PROGRAM LINKING FAILED\n---------" << infoLog << std::endl;
            std::cout << "---------" << std::endl;
        }
    }
};
#endif