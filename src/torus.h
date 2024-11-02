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

std::vector<float> generateCircleVerticesUnrolled(float r2, int segments);
glm::mat4 getCircleModelMatrix(float angle, float r1);

#endif