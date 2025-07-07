#include "cell.h"

glm::i32 to_linear_index(glm::i32 x, glm::i32 y, glm::i32 z, glm::u32 nx, glm::u32 ny, glm::u32 nz) {
    if (x < 0 || y < 0 || z < 0 || glm::u32(x) >= nx || glm::u32(y) >= ny || glm::u32(z) >= nz) {
        return -1; // Out of bounds
    }
    return glm::i32((glm::u32(x) * nz * ny) + (glm::u32(z) * ny) + glm::u32(y));
}

ParticleNeighbors particle_neighbors(
    glm::f32 x, glm::f32 y, glm::f32 z,
    glm::f32 xmin, glm::f32 ymin, glm::f32 zmin,
    glm::f32 xmax, glm::f32 ymax, glm::f32 zmax,
    glm::u32 nx, glm::u32 ny, glm::u32 nz) {
    
    // Check if particle is outside the grid bounds
    if (x < xmin || x >= xmax || y < ymin || y >= ymax || z < zmin || z >= zmax) {
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
    
    // Calculate cell size
    glm::f32 dx = (xmax - xmin) / glm::f32(nx);
    glm::f32 dy = (ymax - ymin) / glm::f32(ny);
    glm::f32 dz = (zmax - zmin) / glm::f32(nz);
    
    // Calculate which cell the particle is in
    glm::i32 cell_x = glm::i32((x - xmin) / dx);
    glm::i32 cell_y = glm::i32((y - ymin) / dy);
    glm::i32 cell_z = glm::i32((z - zmin) / dz);
    
    // Determine which of the 8 surrounding cells to use based on particle position within the cell
    glm::f32 local_x = (x - xmin) / dx - glm::f32(cell_x);
    glm::f32 local_y = (y - ymin) / dy - glm::f32(cell_y);
    glm::f32 local_z = (z - zmin) / dz - glm::f32(cell_z);
    
    // Calculate which cell to base calculations off of
    glm::i32 base_x = cell_x + (local_x > 0.5 ? 0 : -1);
    glm::i32 base_y = cell_y + (local_y > 0.5 ? 0 : -1);
    glm::i32 base_z = cell_z + (local_z > 0.5 ? 0 : -1);
    
    // Calculate the 8 corner cells around the particle
    ParticleNeighbors neighbors;

    neighbors.xp_yp_zp = to_linear_index(base_x + 1, base_y + 1, base_z + 1, nx, ny, nz); // x+, y+, z+
    neighbors.xp_yp_zm = to_linear_index(base_x + 1, base_y + 1, base_z, nx, ny, nz); // x+, y+, z-
    neighbors.xp_ym_zp = to_linear_index(base_x + 1, base_y, base_z + 1, nx, ny, nz); // x+, y-, z+
    neighbors.xp_ym_zm = to_linear_index(base_x + 1, base_y, base_z, nx, ny, nz); // x+, y-, z-
    neighbors.xm_yp_zp = to_linear_index(base_x, base_y + 1, base_z + 1, nx, ny, nz); // x-, y+, z+
    neighbors.xm_yp_zm = to_linear_index(base_x, base_y + 1, base_z, nx, ny, nz); // x-, y+, z-
    neighbors.xm_ym_zp = to_linear_index(base_x, base_y, base_z + 1, nx, ny, nz); // x-, y-, z+
    neighbors.xm_ym_zm = to_linear_index(base_x, base_y, base_z, nx, ny, nz); // x-, y-, z-
    
    return neighbors;
}

CellNeighbors grid_neighbors(glm::u32 idx, glm::u32 nx, glm::u32 ny, glm::u32 nz) {
    // Calculate 3D coordinates from linear index
    // Order: x-major, then z-major, then y-major
    // idx = (x_idx * nz * ny) + (z_idx * ny) + y_idx
    glm::u32 x_idx = idx / (nz * ny);
    glm::u32 remainder = idx % (nz * ny);
    glm::u32 z_idx = remainder / ny;
    glm::u32 y_idx = remainder % ny;
    
    // Calculate neighbor indices
    CellNeighbors neighbors;
    
    // x+1 neighbors
    neighbors.xp_yp_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_yp_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.xp_yp_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.xp_y0_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_y0_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx), nx, ny, nz);
    neighbors.xp_y0_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx), glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.xp_ym_zp = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_ym_z0 = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.xp_ym_zm = to_linear_index(glm::i32(x_idx) + 1, glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, nx, ny, nz);
    
    // x neighbors (same x)
    neighbors.x0_yp_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.x0_yp_z0 = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.x0_yp_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.x0_y0_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx), glm::i32(z_idx) + 1, nx, ny, nz);
    // neighbors.x0_y0_z0 is the current cell (omitted)
    neighbors.x0_y0_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx), glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.x0_ym_zp = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.x0_ym_z0 = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.x0_ym_zm = to_linear_index(glm::i32(x_idx), glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, nx, ny, nz);
    
    // x-1 neighbors
    neighbors.xm_yp_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_yp_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.xm_yp_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) + 1, glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.xm_y0_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_y0_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx), nx, ny, nz);
    neighbors.xm_y0_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx), glm::i32(z_idx) - 1, nx, ny, nz);
    neighbors.xm_ym_zp = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_ym_z0 = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx), nx, ny, nz);
    neighbors.xm_ym_zm = to_linear_index(glm::i32(x_idx) - 1, glm::i32(y_idx) - 1, glm::i32(z_idx) - 1, nx, ny, nz);
    
    return neighbors;
}