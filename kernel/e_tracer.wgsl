struct ETracerParams {
    nCurrentSegments: u32,
    solenoidFlux: f32,
    enableParticleFieldContributions: u32,
    nTracers: u32,
    tracerLength: u32,
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> eTracerTrails: array<vec3<f32>>;
@group(0) @binding(2) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read> currentSegments: array<vec4<f32>>;
@group(0) @binding(5) var<uniform> params: ETracerParams;

@compute @workgroup_size(256)
fn updateTrails(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= params.nTracers) {
        return;
    }

    let traceStart = id * params.tracerLength;
    var loc = eTracerTrails[traceStart];

    for (var i: u32 = 1u; i < params.tracerLength; i++) {
        // Calculate the E field at location
        var E = vec3<f32>(0.0, 0.0, 0.0);
        var B = vec3<f32>(0.0, 0.0, 0.0);

        if (params.enableParticleFieldContributions != 0u) {
            var unused: i32 = -1;
            compute_particle_field_contributions(nParticles, &particlePos, &particleVel, loc, -1, &E, &B, &unused);
        }

        // Calculate the contribution of the central solenoid
        let solenoid_axis = vec3<f32>(0.0, 1.0, 0.0);
        let solenoid_r = vec3<f32>(loc.x, 0.0, loc.z);
        let solenoid_e_mag = params.solenoidFlux / (2.0 * PI * length(solenoid_r));
        E += solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));

        loc += normalize(E) * 0.005 * _M;
        eTracerTrails[traceStart + i] = loc;
    }
} 