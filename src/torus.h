#ifndef __TORUS_H__
#define __TORUS_H__

#include <vector>
#include <glm/glm.hpp>

std::vector<float> generateCircleVertices(float r2, int segments);
glm::mat4 getCircleModelMatrix(float angle, float r1);

#endif