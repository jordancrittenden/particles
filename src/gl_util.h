#ifndef __GL_UTIL_H__
#define __GL_UTIL_H__

#include <GLFW/glfw3.h>

typedef struct GLBufPair {
    GLuint vbo; // vertex buffer
    GLuint vao; // vertex array
} GLBufPair;

GLFWwindow* init_opengl(int windowWidth, int windowHeight);
GLuint create_shader_program(const char* vertexPath, const char* fragmentPath);

#endif