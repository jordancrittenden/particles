#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "cl_util.h"

typedef struct CurrentVector {
    glm::vec4 x;  // position of the current in space
    glm::vec4 dx; // direction of the current in space
    float i;      // current in A
} CurrentVector;

typedef struct TorusProperties {
    float r1 = 1.0f;            // Radius of torus, m
    float r2 = 0.4f;            // Radius of torus cross section, m
    float solenoidR = 0.2f;     // Radius of the central solenoid, m
    float solenoidN = 10;       // Number of turns of the solenoid
    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 20;  // Number of current segments per circle for approximation

    float toroidalI = 50000.0f; // Toroidal current, A
    float solenoidI = 10000.0f; // Central solenoid pulse peak current, A
    float pulseAlpha = 0.01f;   // Pulse exponential parameter
} TorusProperties;

GLBuffers create_torus_buffers(TorusProperties& torus);
void render_torus(GLuint shader, TorusProperties& torus, const GLBuffers& torusBuf, glm::mat4 view, glm::mat4 projection);
std::vector<CurrentVector> get_toroidal_currents(TorusProperties& torus);
float solenoid_pulse_e_field_multiplier(TorusProperties& torus, float t);