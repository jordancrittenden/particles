struct FieldsParams {
    nCells: u32,
    nCurrentSegments: u32,
    solenoidFlux: f32,
    enableParticleFieldContributions: u32,
}

@group(0) @binding(0) var<storage, read> nParticles: u32;
@group(0) @binding(1) var<storage, read> cellLocation: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> eField: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> bField: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read> particlePos: array<vec4<f32>>;
@group(0) @binding(5) var<storage, read> particleVel: array<vec4<f32>>;
@group(0) @binding(6) var<storage, read> currentSegments: array<vec4<f32>>;
@group(0) @binding(7) var<storage, read_write> debug: array<vec4<f32>>;
@group(0) @binding(8) var<uniform> params: FieldsParams;

@compute @workgroup_size(256)
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
        compute_particle_field_contributions(nParticles, particlePos, particleVel, loc, -1, &E, &B, &unused);
    }

    compute_current_field_contributions(currentSegments, params.nCurrentSegments, loc, &B);

    // Calculate the contribution of the central solenoid
    let solenoid_axis = vec3<f32>(0.0, 1.0, 0.0);
    let solenoid_r = vec3<f32>(loc.x, 0.0, loc.z);
    let solenoid_e_mag = params.solenoidFlux / (2.0 * PI * length(solenoid_r));
    E += solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));

    eField[id] = vec4<f32>(E, 0.0);
    bField[id] = vec4<f32>(B, 0.0);

    debug[id] = vec4<f32>(loc, 0.0);
} 