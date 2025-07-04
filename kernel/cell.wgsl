// The index of the neighbors of cell x, y, z
struct Neighbors {
    i32 xp_yp_zp; // x+1, y+1, z+1
    i32 xp_yp_z0; // x+1, y+1, z
    i32 xp_yp_zm; // x+1, y+1, z-1
    i32 xp_y0_zp; // x+1, y,   z+1
    i32 xp_y0_z0; // x+1, y,   z
    i32 xp_y0_zm; // x+1, y,   z-1
    i32 xp_ym_zp; // x+1, y-1, z+1
    i32 xp_ym_z0; // x+1, y-1, z
    i32 xp_ym_zm; // x+1, y-1, z-1

    i32 x0_yp_zp; // x, y+1, z+1
    i32 x0_yp_z0; // x, y+1, z
    i32 x0_yp_zm; // x, y+1, z-1
    i32 x0_y0_zp; // x, y,   z+1
    // omit cell x, y, z
    i32 x0_y0_zm; // x, y,   z-1
    i32 x0_ym_zp; // x, y-1, z+1
    i32 x0_ym_z0; // x, y-1, z
    i32 x0_ym_zm; // x, y-1, z-1

    i32 xm_yp_zp; // x-1, y+1, z+1
    i32 xm_yp_z0; // x-1, y+1, z
    i32 xm_yp_zm; // x-1, y+1, z-1
    i32 xm_y0_zp; // x-1, y,   z+1
    i32 xm_y0_z0; // x-1, y,   z
    i32 xm_y0_zm; // x-1, y,   z-1
    i32 xm_ym_zp; // x-1, y-1, z+1
    i32 xm_ym_z0; // x-1, y-1, z
    i32 xm_ym_zm; // x-1, y-1, z-1
}

fn grid_neighbors(idx: u32, nx: u32, ny: u32, nz: u32) -> Neighbors {
    // Calculate 3D coordinates from linear index
    // Order: x-major, then z-major, then y-major
    // idx = (x_idx * nz * ny) + (z_idx * ny) + y_idx
    let x_idx: u32 = idx / (nz * ny);
    let remainder: u32 = idx % (nz * ny);
    let z_idx: u32 = remainder / ny;
    let y_idx: u32 = remainder % ny;
    
    // Helper function to convert 3D coordinates back to linear index
    fn to_linear_index(x: i32, y: i32, z: i32, nx: u32, ny: u32, nz: u32) -> i32 {
        if (x < 0 || y < 0 || z < 0 || u32(x) >= nx || u32(y) >= ny || u32(z) >= nz) {
            return -1i; // Out of bounds
        }
        return i32((u32(x) * nz * ny) + (u32(z) * ny) + u32(y));
    }
    
    // Calculate neighbor indices
    var neighbors: Neighbors;
    
    // x+1 neighbors
    neighbors.xp_yp_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_yp_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx), nx, ny, nz);
    neighbors.xp_yp_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx) - 1, nx, ny, nz);
    neighbors.xp_y0_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_y0_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx), nx, ny, nz);
    neighbors.xp_y0_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx) - 1, nx, ny, nz);
    neighbors.xp_ym_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.xp_ym_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx), nx, ny, nz);
    neighbors.xp_ym_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx) - 1, nx, ny, nz);
    
    // x neighbors (same x)
    neighbors.x0_yp_zp = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.x0_yp_z0 = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx), nx, ny, nz);
    neighbors.x0_yp_zm = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx) - 1, nx, ny, nz);
    neighbors.x0_y0_zp = to_linear_index(i32(x_idx), i32(y_idx), i32(z_idx) + 1, nx, ny, nz);
    // neighbors.x0_y0_z0 is the current cell (omitted)
    neighbors.x0_y0_zm = to_linear_index(i32(x_idx), i32(y_idx), i32(z_idx) - 1, nx, ny, nz);
    neighbors.x0_ym_zp = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.x0_ym_z0 = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx), nx, ny, nz);
    neighbors.x0_ym_zm = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx) - 1, nx, ny, nz);
    
    // x-1 neighbors
    neighbors.xm_yp_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_yp_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx), nx, ny, nz);
    neighbors.xm_yp_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx) - 1, nx, ny, nz);
    neighbors.xm_y0_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_y0_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx), nx, ny, nz);
    neighbors.xm_y0_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx) - 1, nx, ny, nz);
    neighbors.xm_ym_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx) + 1, nx, ny, nz);
    neighbors.xm_ym_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx), nx, ny, nz);
    neighbors.xm_ym_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx) - 1, nx, ny, nz);
    
    return neighbors;
}