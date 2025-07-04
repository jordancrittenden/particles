#pragma once

#include <glm/glm.hpp>

struct Cell {
    glm::f32vec4 pos; // [centerX, centerY, centerZ, isActive]
    glm::f32vec3 min; // [minX, minY, minZ]
    glm::f32vec3 max; // [maxX, maxY, maxZ]
};