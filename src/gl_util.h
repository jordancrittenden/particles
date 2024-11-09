#pragma once

#define GL_SILENCE_DEPRECATION
#define GLM_ENABLE_EXPERIMENT

#include <GLFW/glfw3.h>

typedef struct GLBuffers {
    GLuint vbo; // vertex buffer
    GLuint vao; // vertex array
    GLuint instance_vbo; // instance buffer
} GLBufPair;

GLFWwindow* init_opengl(int windowWidth, int windowHeight);
GLuint create_shader_program(const char* vertexPath, const char* fragmentPath);