#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

std::string loadShaderSource(const char* filepath) {
    std::ifstream file;
    std::stringstream buffer;

    // Ensure ifstream objects can throw exceptions
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(filepath);
        buffer << file.rdbuf();  // Read the entire file into the buffer
        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filepath << std::endl;
    }

    return buffer.str();  // Return the content as a string
}

GLuint compileShader(const char* filepath, GLenum shaderType) {
    // Load the shader source code from file
    std::string shaderCode = loadShaderSource(filepath);  // Helper function to read file content
    const char* shaderSource = shaderCode.c_str();

    // Create and compile the shader
    GLuint shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, &shaderSource, nullptr);  // Pass the shader source code
    glCompileShader(shaderID);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shaderID;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    // Compile the vertex shader
    GLuint vertexShader = compileShader(vertexPath, GL_VERTEX_SHADER);
    
    // Compile the fragment shader
    GLuint fragmentShader = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

    // Create shader program and link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Clean up, delete shaders as they are now linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}