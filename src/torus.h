#pragma once

#include "gl_util.h"

typedef struct TorusParameters {
    float r1 = 1.0f;            // Radius of torus, m
    float r2 = 0.4f;            // Radius of torus cross section, m

    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 20;  // Number of current segments per circle for approximation
    float toroidalI = 50000.0f; // Toroidal current, A

    float solenoidR = 0.2f;     // Radius of the central solenoid, m
    float solenoidFlux = 0.3f;  // Central solenoid magnetic flux, V*s
} TorusParameters;

glm::mat4 get_coil_model_matrix(float angle, float r1);
GLBuffers create_torus_buffers(TorusParameters& parameters);
void render_torus(GLuint shader, const GLBuffers& torusBuf, TorusParameters& parameters, glm::mat4 view, glm::mat4 projection);