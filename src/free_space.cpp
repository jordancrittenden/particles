#include <iostream>
#include <glm/glm.hpp>
#include "cell.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

glm::f32vec4 free_space_rand_particle_position(glm::f32vec3 minCoord, glm::f32vec3 maxCoord) {
    float x = rand_range(minCoord.x, maxCoord.x);
    float y = rand_range(minCoord.y, maxCoord.y);
    float z = rand_range(minCoord.z, maxCoord.z);

    // [x, y, z, unused]
    return glm::f32vec4 { x, y, z, 0.0f };
}

std::vector<Cell> get_free_space_grid_cells(glm::f32vec3 minCoord, glm::f32vec3 maxCoord, glm::f32 dx) {
    std::vector<Cell> cells;
    for (float x = minCoord.x; x <= maxCoord.x; x += dx) {
        for (float z = minCoord.z; z <= maxCoord.z; z += dx) {
            for (float y = minCoord.y; y <= maxCoord.y; y += dx) {
                Cell cell;
                cell.pos = glm::f32vec4 { x, y, z, 1.0f };
                cell.min = glm::f32vec3 { x - dx/2.0f, y - dx/2.0f, z - dx/2.0f };
                cell.max = glm::f32vec3 { x + dx/2.0f, y + dx/2.0f, z + dx/2.0f };
                cells.push_back(cell);
            }
        }
    }
    return cells;
}