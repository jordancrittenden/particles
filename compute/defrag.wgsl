struct DefragParams {
    maxParticles: u32,
}

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(3) var<uniform> params: DefragParams;

@compute @workgroup_size(1)
fn defrag() {
    var write_head: u32 = 0u;
    
    for (var i: u32 = 0u; i < params.maxParticles; i++) {
        let species = particlePos[i].w;
        if (species == 0.0) {
            continue;
        }

        if (write_head < i) {
            particlePos[write_head] = particlePos[i];
            particleVel[write_head] = particleVel[i];
            particlePos[i] = vec4<f32>(0.0, 0.0, 0.0, 0.0);
            particleVel[i] = vec4<f32>(0.0, 0.0, 0.0, 0.0);
        }

        write_head++;
    }

    nParticles = write_head;
} 