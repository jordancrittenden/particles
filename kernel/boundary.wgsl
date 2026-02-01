// Axis-aligned box boundary: particles that exit are wrapped to the opposite side (periodic BC).
struct BoundaryParams {
    x_min: f32,
    x_max: f32,
    y_min: f32,
    y_max: f32,
    z_min: f32,
    z_max: f32,
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(3) var<uniform> params: BoundaryParams;

fn wrap_axis(pos_axis: f32, min_axis: f32, max_axis: f32) -> f32 {
    let extent = max_axis - min_axis;
    if (extent <= 0.0) {
        return pos_axis;
    }
    let d = pos_axis - min_axis;
    return min_axis + d - extent * floor(d / extent);
}

@compute @workgroup_size(256)
fn applyBoundary(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= nParticles) {
        return;
    }

    let species = particlePos[id].w;
    if (species == 0.0) {
        return; // inactive particle
    }

    let pos = vec3<f32>(particlePos[id].xyz);

    let new_x = wrap_axis(pos.x, params.x_min, params.x_max);
    let new_y = wrap_axis(pos.y, params.y_min, params.y_max);
    let new_z = wrap_axis(pos.z, params.z_min, params.z_max);

    particlePos[id] = vec4<f32>(new_x, new_y, new_z, species);
}
