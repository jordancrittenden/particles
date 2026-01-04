struct ComputeFieldsParams {
    nCells: u32,
    nCurrentSegments: u32,
    solenoidFlux: f32,
    enableParticleFieldContributions: u32,
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read> cellLocation: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> eField: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> bField: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(5) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(6) var<storage, read> currentSegments: array<vec4<f32>>;
@group(0) @binding(7) var<storage, read_write> debug: array<vec4<f32>>;
@group(0) @binding(8) var<uniform> params: ComputeFieldsParams;

@compute @workgroup_size(256)
// Computes the value of the E and B field at each cell location
fn computeFields(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= params.nCells) {
        return;
    }

    // Extract position for this cell
    let loc = vec3<f32>(cellLocation[id].xyz);

    // Calculate the E and B field at location
    var E = vec3<f32>(0.0, 0.0, 0.0);
    var B = vec3<f32>(0.0, 0.0, 0.0);

    if (params.enableParticleFieldContributions != 0u) {
        var unused: i32 = -1;
        compute_particle_field_contributions(nParticles, &particlePos, &particleVel, loc, -1, &E, &B, &unused);
    }

    // Calculate the contribution of the currents
    B += compute_currents_b_field(&currentSegments, params.nCurrentSegments, loc);

    // Calculate the contribution of the central solenoid
    E += compute_solenoid_e_field(params.solenoidFlux, loc);

    eField[id] = vec4<f32>(E, 0.0);
    bField[id] = vec4<f32>(B, 0.0);

    debug[id] = vec4<f32>(loc, 0.0);
} 