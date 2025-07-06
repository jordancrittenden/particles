#pragma once

#include <glm/glm.hpp>

struct Cell {
    glm::f32vec4 pos; // [centerX, centerY, centerZ, isActive]
    glm::f32vec3 min; // [minX, minY, minZ]
    glm::f32vec3 max; // [maxX, maxY, maxZ]
};

// The indices of the neighbors of cell x, y, z, assuming the cells are a grid ordered in x-major, z-major, y-major order
struct CellNeighbors {
    glm::i32 xp_yp_zp; // x+1, y+1, z+1
    glm::i32 xp_yp_z0; // x+1, y+1, z
    glm::i32 xp_yp_zm; // x+1, y+1, z-1
    glm::i32 xp_y0_zp; // x+1, y,   z+1
    glm::i32 xp_y0_z0; // x+1, y,   z
    glm::i32 xp_y0_zm; // x+1, y,   z-1
    glm::i32 xp_ym_zp; // x+1, y-1, z+1
    glm::i32 xp_ym_z0; // x+1, y-1, z
    glm::i32 xp_ym_zm; // x+1, y-1, z-1

    glm::i32 x0_yp_zp; // x, y+1, z+1
    glm::i32 x0_yp_z0; // x, y+1, z
    glm::i32 x0_yp_zm; // x, y+1, z-1
    glm::i32 x0_y0_zp; // x, y,   z+1
    // omit cell x, y, z
    glm::i32 x0_y0_zm; // x, y,   z-1
    glm::i32 x0_ym_zp; // x, y-1, z+1
    glm::i32 x0_ym_z0; // x, y-1, z
    glm::i32 x0_ym_zm; // x, y-1, z-1

    glm::i32 xm_yp_zp; // x-1, y+1, z+1
    glm::i32 xm_yp_z0; // x-1, y+1, z
    glm::i32 xm_yp_zm; // x-1, y+1, z-1
    glm::i32 xm_y0_zp; // x-1, y,   z+1
    glm::i32 xm_y0_z0; // x-1, y,   z
    glm::i32 xm_y0_zm; // x-1, y,   z-1
    glm::i32 xm_ym_zp; // x-1, y-1, z+1
    glm::i32 xm_ym_z0; // x-1, y-1, z
    glm::i32 xm_ym_zm; // x-1, y-1, z-1
};

CellNeighbors grid_neighbors(glm::u32 idx, glm::u32 nx, glm::u32 ny, glm::u32 nz);