struct ETracerParams {
    nCurrentSegments: u32,
    solenoidFlux: f32,
    enableParticleFieldContributions: u32,
    nTracers: u32,
    tracerLength: u32,
    curTraceIdx: u32,
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
    if (params.curTraceIdx == 0) {
        return;
    }

    // Calculate the E field at location
    var E = vec3<f32>(0.0, 0.0, 0.0);
    var B = vec3<f32>(0.0, 0.0, 0.0);

    let traceStart = id * params.tracerLength;
    let pLoc = eTracerTrails[traceStart + params.curTraceIdx - 1];

    if (params.enableParticleFieldContributions != 0u) {
        var unused: i32 = -1;
        compute_particle_field_contributions(nParticles, &particlePos, &particleVel, pLoc, -1, &E, &B, &unused);
    }

    // Calculate the contribution of the central solenoid
    E += compute_solenoid_e_field(params.solenoidFlux, pLoc);

    eTracerTrails[traceStart + params.curTraceIdx] = pLoc + normalize(E) * 0.005 * _M;
} 