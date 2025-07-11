// Torus parameters
struct TorusWallParams {
    r1: f32,  // Major radius of torus (distance from center to torus centerline)
    r2: f32,  // Minor radius of torus (radius of torus cross section)
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(3) var<uniform> params: TorusWallParams;

@compute @workgroup_size(256)
fn checkWallInteractions(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= nParticles) {
        return;
    }

    let species = particlePos[id].w;
    if (species == 0.0) {
        return; // inactive particle
    }

    let pos = vec3<f32>(particlePos[id].xyz);
    
    // Calculate distance from torus centerline
    // The torus centerline is a circle of radius r1 in the xz-plane
    let radialDistFromOrigin = sqrt(pos.x * pos.x + pos.z * pos.z);
    let radialDistFromTorusCenterline = radialDistFromOrigin - params.r1;
    
    // Calculate distance from torus centerline (including y-coordinate)
    let distFromTorusCenterline = sqrt(radialDistFromTorusCenterline * radialDistFromTorusCenterline + pos.y * pos.y);
    
    // Check if particle has hit the torus wall
    if (distFromTorusCenterline >= params.r2) {
        // Particle has hit the wall, set species to 0 (inactive)
        particlePos[id] = vec4<f32>(pos, 0.0);
    }
}
