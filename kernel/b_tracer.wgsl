struct BTracerParams {
    nCurrentSegments: u32,
    enableParticleFieldContributions: u32,
    nTracers: u32,
    tracerLength: u32,
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> bTracerTrails: array<vec3<f32>>;
@group(0) @binding(2) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read> currentSegments: array<vec4<f32>>;
@group(0) @binding(5) var<uniform> params: BTracerParams;

@compute @workgroup_size(256)
fn updateTrails(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= params.nTracers) {
        return;
    }

    let traceStart = id * params.tracerLength;
    var loc = bTracerTrails[traceStart];

    for (var i: u32 = 1u; i < params.tracerLength; i++) {
        // Calculate the B field at location
        var E = vec3<f32>(0.0, 0.0, 0.0);
        var B = vec3<f32>(0.0, 0.0, 0.0);

        if (params.enableParticleFieldContributions != 0u) {
            var unused: i32 = -1;
            compute_particle_field_contributions(nParticles, &particlePos, &particleVel, loc, -1, &E, &B, &unused);
        }

        compute_current_field_contributions(&currentSegments, params.nCurrentSegments, loc, &B);

        loc += normalize(B) * 0.005 * _M;
        bTracerTrails[traceStart + i] = loc;
    }
} 