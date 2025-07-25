#include <iostream>
#include <glm/glm.hpp>
#include "mesh.h"

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

std::vector<Cell> get_free_space_mesh_cells(glm::f32vec3 minCoord, glm::f32vec3 maxCoord, glm::f32vec3 size, MeshProperties& mesh) {
    std::vector<Cell> cells;
    glm::u32 nx = 0, ny = 0, nz = 0;
    bool countZ = true, countY = true;
    for (float x = minCoord.x; x <= maxCoord.x; x += size.x) {
        for (float z = minCoord.z; z <= maxCoord.z; z += size.z) {
            for (float y = minCoord.y; y <= maxCoord.y; y += size.y) {
                Cell cell;
                cell.pos = glm::f32vec4 { x, y, z, 1.0f };
                cell.min = glm::f32vec3 { x - size.x/2.0f, y - size.y/2.0f, z - size.z/2.0f };
                cell.max = glm::f32vec3 { x + size.x/2.0f, y + size.y/2.0f, z + size.z/2.0f };
                cells.push_back(cell);
                if (countY) ny++;
            }
            if (countZ) nz++;
            countY = false;
        }
        nx++;
        countZ = false;
    }
    mesh.dim = glm::u32vec3 { nx, ny, nz };
    mesh.cell_size = size;
    mesh.min = minCoord;
    mesh.max = maxCoord;
    
    return cells;
}