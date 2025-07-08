#include "mesh.h"

glm::i32 to_linear_index(glm::i32 x, glm::i32 y, glm::i32 z, glm::u32vec3 dim) {
    if (x < 0 || y < 0 || z < 0 || glm::u32(x) >= dim.x || glm::u32(y) >= dim.y || glm::u32(z) >= dim.z) {
        return -1; // Out of bounds
    }
    return glm::i32((glm::u32(x) * dim.z * dim.y) + (glm::u32(z) * dim.y) + glm::u32(y));
}

ParticleNeighbors particle_neighbors(glm::f32vec3 pos, const MeshProperties& mesh) {
    // Check if particle is outside the mesh bounds
    if (pos.x < mesh.min.x || pos.x >= mesh.max.x || pos.y < mesh.min.y || pos.y >= mesh.max.y || pos.z < mesh.min.z || pos.z >= mesh.max.z) {
        ParticleNeighbors neighbors;
        neighbors.xp_yp_zp = -1;
        neighbors.xp_yp_zm = -1;
        neighbors.xp_ym_zp = -1;
        neighbors.xp_ym_zm = -1;
        neighbors.xm_yp_zp = -1;
        neighbors.xm_yp_zm = -1;
        neighbors.xm_ym_zp = -1;
        neighbors.xm_ym_zm = -1;
        return neighbors;
    }
    
    // Calculate which cell the particle is in
    glm::i32 cell_x = glm::i32((pos.x - mesh.min.x) / mesh.cell_size.x);
    glm::i32 cell_y = glm::i32((pos.y - mesh.min.y) / mesh.cell_size.y);
    glm::i32 cell_z = glm::i32((pos.z - mesh.min.z) / mesh.cell_size.z);
    
    // Determine which of the 8 surrounding cells to use based on particle position within the cell
    glm::f32 local_x = (pos.x - mesh.min.x) / mesh.cell_size.x - glm::f32(cell_x);
    glm::f32 local_y = (pos.y - mesh.min.y) / mesh.cell_size.y - glm::f32(cell_y);
    glm::f32 local_z = (pos.z - mesh.min.z) / mesh.cell_size.z - glm::f32(cell_z);
    
    // Calculate which cell to base calculations off of
    glm::i32 base_x = cell_x + (local_x > 0.5 ? 0 : -1);
    glm::i32 base_y = cell_y + (local_y > 0.5 ? 0 : -1);
    glm::i32 base_z = cell_z + (local_z > 0.5 ? 0 : -1);
    
    // Calculate the 8 corner cells around the particle
    ParticleNeighbors neighbors;

    neighbors.xp_yp_zp = to_linear_index(base_x + 1, base_y + 1, base_z + 1, mesh.dim); // x+, y+, z+
    neighbors.xp_yp_zm = to_linear_index(base_x + 1, base_y + 1, base_z, mesh.dim); // x+, y+, z-
    neighbors.xp_ym_zp = to_linear_index(base_x + 1, base_y, base_z + 1, mesh.dim); // x+, y-, z+
    neighbors.xp_ym_zm = to_linear_index(base_x + 1, base_y, base_z, mesh.dim); // x+, y-, z-
    neighbors.xm_yp_zp = to_linear_index(base_x, base_y + 1, base_z + 1, mesh.dim); // x-, y+, z+
    neighbors.xm_yp_zm = to_linear_index(base_x, base_y + 1, base_z, mesh.dim); // x-, y+, z-
    neighbors.xm_ym_zp = to_linear_index(base_x, base_y, base_z + 1, mesh.dim); // x-, y-, z+
    neighbors.xm_ym_zm = to_linear_index(base_x, base_y, base_z, mesh.dim); // x-, y-, z-
    
    return neighbors;
}

CellNeighbors cell_neighbors(glm::u32 idx, glm::u32vec3 dim) {
    // Calculate 3D coordinates from linear index
    // Order: x-major, then z-major, then y-major
    // idx = (x_idx * nz * ny) + (z_idx * ny) + y_idx
    glm::u32 x_idx = idx / (dim.z * dim.y);
    glm::u32 remainder = idx % (dim.z * dim.y);
    glm::u32 z_idx = remainder / dim.y;
    glm::u32 y_idx = remainder % dim.y;
    
    // Calculate neighbor indices
    CellNeighbors neighbors;
    
    // x+1 neighbors
    neighbors.xp_yp_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, dim);
    neighbors.xp_yp_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx), dim);
    neighbors.xp_yp_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, dim);
    neighbors.xp_y0_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx) + 1, dim);
    neighbors.xp_y0_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx), dim);
    neighbors.xp_y0_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx) - 1, dim);
    neighbors.xp_ym_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, dim);
    neighbors.xp_ym_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx), dim);
    neighbors.xp_ym_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, dim);
    
    // x neighbors (same x)
    neighbors.x0_yp_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, dim);
    neighbors.x0_yp_z0 = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx), dim);
    neighbors.x0_yp_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, dim);
    neighbors.x0_y0_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx), glm::i32(z_idx) + 1, dim);
    // neighbors.x0_y0_z0 is the current cell (omitted)
    neighbors.x0_y0_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx), glm::i32(z_idx) - 1, dim);
    neighbors.x0_ym_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, dim);
    neighbors.x0_ym_z0 = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx), dim);
    neighbors.x0_ym_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, dim);
    
    // x-1 neighbors
    neighbors.xm_yp_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, dim);
    neighbors.xm_yp_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx), dim);
    neighbors.xm_yp_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, dim);
    neighbors.xm_y0_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx) + 1, dim);
    neighbors.xm_y0_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx), dim);
    neighbors.xm_y0_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx) - 1, dim);
    neighbors.xm_ym_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, dim);
    neighbors.xm_ym_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx), dim);
    neighbors.xm_ym_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, dim);
    
    return neighbors;
}