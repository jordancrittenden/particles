#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"
#include "cl_util.h"
#include "state.h"
#include "current_segment.h"

typedef struct TorusProperties {
    float r1 = 1.0f;            // Radius of torus, m
    float r2 = 0.4f;            // Radius of torus cross section, m

    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 20;  // Number of current segments per circle for approximation
    float toroidalI = 50000.0f; // Toroidal current, A

    float solenoidR = 0.2f;     // Radius of the central solenoid, m
    float solenoidFlux = 0.3f;  // Central solenoid magnetic flux, V*s
} TorusProperties;

GLBuffers create_torus_buffers(const TorusProperties& torus);
void render_torus(GLuint shader, const TorusProperties& torus, const GLBuffers& torusBuf, glm::mat4 view, glm::mat4 projection);
std::vector<CurrentVector> get_toroidal_currents(const TorusProperties& torus);
std::vector<Cell> get_torus_grid_cells(const TorusProperties& torus, glm::vec3 minCoord, glm::vec3 maxCoord, float dx);