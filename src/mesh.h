#pragma once

#include <glm/glm.hpp>

// The full mesh, including active and inactive cells. The mesh is always a grid, to support easier
// calculation of cell indices. However, the active cells may be a subset of the full mesh, such as the
// torus area of a tokamak.
struct MeshProperties {
    glm::f32vec3 min; // minimum cell center
    glm::f32vec3 max; // maximum cell center
    glm::u32vec3 dim; // number of cells in each dimension
    glm::f32vec3 cell_size; // cell size
};

// Active cells define the boundary of the active plasma region. When a particle reaches the boundary,
// it is reflected or absorbed, depending on the particle species.
struct Cell {
    glm::f32vec4 pos; // [centerX, centerY, centerZ, isActive]
    glm::f32vec3 min; // [minX, minY, minZ]
    glm::f32vec3 max; // [maxX, maxY, maxZ]
};

struct CellNeighbors {
    glm::i32 xp_yp_zp; // x+, y+, z+
    glm::i32 xp_yp_zm; // x+, y+, z-
    glm::i32 xp_ym_zp; // x+, y-, z+
    glm::i32 xp_ym_zm; // x+, y-, z-
    glm::i32 xm_yp_zp; // x-, y+, z+
    glm::i32 xm_yp_zm; // x-, y+, z-
    glm::i32 xm_ym_zp; // x-, y-, z+
    glm::i32 xm_ym_zm; // x-, y-, z-
};

CellNeighbors cell_neighbors(glm::f32vec3 pos, const MeshProperties& mesh);