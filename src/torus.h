#ifndef __TORUS_H__
#define __TORUS_H__

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
    float r2 = 0.2f;            // Radius of torus cross section, m
    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 10;  // Number of current segments per circle for approximation

    float toroidalI = 50000.0;  // Toroidal current, A
} TorusProperties;

std::vector<float> generate_coil_vertices_unrolled(float r2, int segments);
glm::mat4 get_coil_model_matrix(float angle, float r1);
std::vector<CurrentVector> get_toroidal_currents(TorusProperties& torus);

#endif