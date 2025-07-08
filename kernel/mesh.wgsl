struct MeshProperties {
    min: vec3<f32>,       // minimum cell center
    max: vec3<f32>,       // maximum cell center
    dim: vec3<u32>,       // number of cells in each dimension
    cell_size: vec3<f32>, // cell size
}

// The indices of the neighbors of cell x, y, z, assuming the cells are a mesh ordered in x-major, z-major, y-major order
struct CellNeighbors {
    xp_yp_zp: i32, // x+1, y+1, z+1
    xp_yp_z0: i32, // x+1, y+1, z
    xp_yp_zm: i32, // x+1, y+1, z-1
    xp_y0_zp: i32, // x+1, y,   z+1
    xp_y0_z0: i32, // x+1, y,   z
    xp_y0_zm: i32, // x+1, y,   z-1
    xp_ym_zp: i32, // x+1, y-1, z+1
    xp_ym_z0: i32, // x+1, y-1, z
    xp_ym_zm: i32, // x+1, y-1, z-1

    x0_yp_zp: i32, // x, y+1, z+1
    x0_yp_z0: i32, // x, y+1, z
    x0_yp_zm: i32, // x, y+1, z-1
    x0_y0_zp: i32, // x, y,   z+1
    x0_y0_z0: i32, // x, y,   z+1
    x0_y0_zm: i32, // x, y,   z-1
    x0_ym_zp: i32, // x, y-1, z+1
    x0_ym_z0: i32, // x, y-1, z
    x0_ym_zm: i32, // x, y-1, z-1

    xm_yp_zp: i32, // x-1, y+1, z+1
    xm_yp_z0: i32, // x-1, y+1, z
    xm_yp_zm: i32, // x-1, y+1, z-1
    xm_y0_zp: i32, // x-1, y,   z+1
    xm_y0_z0: i32, // x-1, y,   z
    xm_y0_zm: i32, // x-1, y,   z-1
    xm_ym_zp: i32, // x-1, y-1, z+1
    xm_ym_z0: i32, // x-1, y-1, z
    xm_ym_zm: i32, // x-1, y-1, z-1
}

struct ParticleNeighbors {
    xp_yp_zp: i32, // x+, y+, z+
    xp_yp_zm: i32, // x+, y+, z-
    xp_ym_zp: i32, // x+, y-, z+
    xp_ym_zm: i32, // x+, y-, z-
    xm_yp_zp: i32, // x-, y+, z+
    xm_yp_zm: i32, // x-, y+, z-
    xm_ym_zp: i32, // x-, y-, z+
    xm_ym_zm: i32, // x-, y-, z-
}
    
// Helper function to convert 3D coordinates back to linear index
fn to_linear_index(x: i32, y: i32, z: i32, dim: vec3<u32>) -> i32 {
    if (x < 0 || y < 0 || z < 0 || u32(x) >= dim.x || u32(y) >= dim.y || u32(z) >= dim.z) {
        return -1i; // Out of bounds
    }
    return i32((u32(x) * dim.z * dim.y) + (u32(z) * dim.y) + u32(y));
}

fn cell_neighbors(idx: u32, dim: vec3<u32>) -> CellNeighbors {
    // Calculate 3D coordinates from linear index
    // Order: x-major, then z-major, then y-major
    // idx = (x_idx * dim.z * dim.y) + (z_idx * dim.y) + y_idx
    let x_idx: u32 = idx / (dim.z * dim.y);
    let remainder: u32 = idx % (dim.z * dim.y);
    let z_idx: u32 = remainder / dim.y;
    let y_idx: u32 = remainder % dim.y;
    
    // Calculate neighbor indices
    var neighbors: CellNeighbors;
    
    // x+1 neighbors
    neighbors.xp_yp_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx) + 1, dim);
    neighbors.xp_yp_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx), dim);
    neighbors.xp_yp_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx) + 1, i32(z_idx) - 1, dim);
    neighbors.xp_y0_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx) + 1, dim);
    neighbors.xp_y0_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx), dim);
    neighbors.xp_y0_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx), i32(z_idx) - 1, dim);
    neighbors.xp_ym_zp = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx) + 1, dim);
    neighbors.xp_ym_z0 = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx), dim);
    neighbors.xp_ym_zm = to_linear_index(i32(x_idx) + 1, i32(y_idx) - 1, i32(z_idx) - 1, dim);
    
    // x neighbors (same x)
    neighbors.x0_yp_zp = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx) + 1, dim);
    neighbors.x0_yp_z0 = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx), dim);
    neighbors.x0_yp_zm = to_linear_index(i32(x_idx), i32(y_idx) + 1, i32(z_idx) - 1, dim);
    neighbors.x0_y0_zp = to_linear_index(i32(x_idx), i32(y_idx), i32(z_idx) + 1, dim);
    neighbors.x0_y0_z0 = to_linear_index(i32(x_idx), i32(y_idx), i32(z_idx), dim);
    neighbors.x0_y0_zm = to_linear_index(i32(x_idx), i32(y_idx), i32(z_idx) - 1, dim);
    neighbors.x0_ym_zp = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx) + 1, dim);
    neighbors.x0_ym_z0 = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx), dim);
    neighbors.x0_ym_zm = to_linear_index(i32(x_idx), i32(y_idx) - 1, i32(z_idx) - 1, dim);
    
    // x-1 neighbors
    neighbors.xm_yp_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx) + 1, dim);
    neighbors.xm_yp_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx), dim);
    neighbors.xm_yp_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx) + 1, i32(z_idx) - 1, dim);
    neighbors.xm_y0_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx) + 1, dim);
    neighbors.xm_y0_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx), dim);
    neighbors.xm_y0_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx), i32(z_idx) - 1, dim);
    neighbors.xm_ym_zp = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx) + 1, dim);
    neighbors.xm_ym_z0 = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx), dim);
    neighbors.xm_ym_zm = to_linear_index(i32(x_idx) - 1, i32(y_idx) - 1, i32(z_idx) - 1, dim);
    
    return neighbors;
}

fn particle_neighbors(pos: vec3<f32>, mesh: ptr<uniform, MeshProperties>) -> ParticleNeighbors {
    // Check if particle is outside the mesh bounds
    if (pos.x < (*mesh).min.x || pos.x >= (*mesh).max.x ||
        pos.y < (*mesh).min.y || pos.y >= (*mesh).max.y ||
        pos.z < (*mesh).min.z || pos.z >= (*mesh).max.z) {
        var neighbors: ParticleNeighbors;
        neighbors.xp_yp_zp = -1i;
        neighbors.xp_yp_zm = -1i;
        neighbors.xp_ym_zp = -1i;
        neighbors.xp_ym_zm = -1i;
        neighbors.xm_yp_zp = -1i;
        neighbors.xm_yp_zm = -1i;
        neighbors.xm_ym_zp = -1i;
        neighbors.xm_ym_zm = -1i;
        return neighbors;
    }
    
    // Calculate which cell the particle is in and local position within that cell
    let cell_idx_frac: vec3<f32> = (pos - (*mesh).min) / (*mesh).cell_size;                                    // [x.aaa, y.bbb, z.ccc]
    let cell_idx_i32: vec3<i32> = vec3<i32>(i32(cell_idx_frac.x), i32(cell_idx_frac.y), i32(cell_idx_frac.z)); // [x, y, z]
    let cell_idx_f32: vec3<f32> = vec3<f32>(f32(cell_idx_i32.x), f32(cell_idx_i32.y), f32(cell_idx_i32.z));    // [x.000, y.000, z.000]
    let local_pos: vec3<f32> = cell_idx_frac - cell_idx_f32;                                                   // [0.aaa, 0.bbb, 0.ccc]
    
    // Calculate which cell to base calculations off of
    let base_x: i32 = cell_idx_i32.x + select(-1, 0, local_pos.x > 0.5);
    let base_y: i32 = cell_idx_i32.y + select(-1, 0, local_pos.y > 0.5);
    let base_z: i32 = cell_idx_i32.z + select(-1, 0, local_pos.z > 0.5);
    
    // Calculate the 8 corner cells around the particle
    var neighbors: ParticleNeighbors;

    neighbors.xp_yp_zp = to_linear_index(base_x + 1, base_y + 1, base_z + 1, (*mesh).dim); // x+, y+, z+
    neighbors.xp_yp_zm = to_linear_index(base_x + 1, base_y + 1, base_z, (*mesh).dim); // x+, y+, z-
    neighbors.xp_ym_zp = to_linear_index(base_x + 1, base_y, base_z + 1, (*mesh).dim); // x+, y-, z+
    neighbors.xp_ym_zm = to_linear_index(base_x + 1, base_y, base_z, (*mesh).dim); // x+, y-, z-
    neighbors.xm_yp_zp = to_linear_index(base_x, base_y + 1, base_z + 1, (*mesh).dim); // x-, y+, z+
    neighbors.xm_yp_zm = to_linear_index(base_x, base_y + 1, base_z, (*mesh).dim); // x-, y+, z-
    neighbors.xm_ym_zp = to_linear_index(base_x, base_y, base_z + 1, (*mesh).dim); // x-, y-, z+
    neighbors.xm_ym_zm = to_linear_index(base_x, base_y, base_z, (*mesh).dim); // x-, y-, z-
    
    return neighbors;
}

fn interp(
    mesh: ptr<uniform, MeshProperties>,
    field: ptr<storage, array<vec4<f32>>, read_write>,
    pos: vec3<f32>
) -> vec4<f32> {
    let neighbors: ParticleNeighbors = particle_neighbors(pos, mesh);
    
    // Check if particle is outside mesh bounds
    if (neighbors.xp_yp_zp == -1i) {
        return vec4<f32>(0.0, 0.0, 0.0, 0.0); // Return zero if outside bounds
    }
    
    // Calculate which cell the particle is in and local position within that cell
    let cell_idx_frac: vec3<f32> = (pos - (*mesh).min) / (*mesh).cell_size;                                          // [x.aaa, y.bbb, z.ccc]
    let cell_idx_f32: vec3<f32> = vec3<f32>(floor(cell_idx_frac.x), floor(cell_idx_frac.y), floor(cell_idx_frac.z)); // [x.000, y.000, z.000]
    let local_pos: vec3<f32> = cell_idx_frac - cell_idx_f32;                                                         // [0.aaa, 0.bbb, 0.ccc]
    
    // Calculate interpolation weights for trilinear interpolation
    let wx: f32 = local_pos.x;
    let wy: f32 = local_pos.y;
    let wz: f32 = local_pos.z;
    
    // Get field values at the 8 corner cells
    let v000: vec4<f32> = (*field)[u32(neighbors.xm_ym_zm)];
    let v001: vec4<f32> = (*field)[u32(neighbors.xm_ym_zp)];
    let v010: vec4<f32> = (*field)[u32(neighbors.xm_yp_zm)];
    let v011: vec4<f32> = (*field)[u32(neighbors.xm_yp_zp)];
    let v100: vec4<f32> = (*field)[u32(neighbors.xp_ym_zm)];
    let v101: vec4<f32> = (*field)[u32(neighbors.xp_ym_zp)];
    let v110: vec4<f32> = (*field)[u32(neighbors.xp_yp_zm)];
    let v111: vec4<f32> = (*field)[u32(neighbors.xp_yp_zp)];
    
    // Perform trilinear interpolation
    // Interpolate along x-axis first
    let v00: vec4<f32> = mix(v000, v100, wx);
    let v01: vec4<f32> = mix(v001, v101, wx);
    let v10: vec4<f32> = mix(v010, v110, wx);
    let v11: vec4<f32> = mix(v011, v111, wx);
    
    // Interpolate along y-axis
    let v0: vec4<f32> = mix(v00, v10, wy);
    let v1: vec4<f32> = mix(v01, v11, wy);
    
    // Interpolate along z-axis
    let result: vec4<f32> = mix(v0, v1, wz);
    
    return result;
}