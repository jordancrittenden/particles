#pragma once

#include "gl_util.h"

typedef struct TorusParameters {
    float r1 = 1.0f;            // Radius of torus, m
    float r2 = 0.4f;            // Radius of torus cross section, m

    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 20;  // Number of current segments per circle for approximation
    float toroidalI = 50000.0f; // Toroidal current, A
} TorusParameters;

glm::mat4 get_coil_model_matrix(float angle, float r1);
void render_torus(GLuint shader, const GLBuffers& ringBuf, const TorusParameters& parameters, float current, glm::mat4 view, glm::mat4 projection);