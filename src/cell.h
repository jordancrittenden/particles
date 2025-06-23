#pragma once

#include <glm/glm.hpp>

typedef struct Cell {
    glm::f32vec4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;