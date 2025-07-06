#include "cell.h"

glm::i32 to_linear_index(glm::i32 x, glm::i32 y, glm::i32 z, glm::u32 nx, glm::u32 ny, glm::u32 nz) {
    if (x < 0 || y < 0 || z < 0 || glm::u32(x) >= nx || glm::u32(y) >= ny || glm::u32(z) >= nz) {
        return -1; // Out of bounds
    }
    return glm::i32((glm::u32(x) * nz * ny) + (glm::u32(z) * ny) + glm::u32(y));
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