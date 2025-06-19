__kernel void updateTrails(
    __global uint* nParticles,
    __global float3* bTracerTrails, 
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    const uint nCurrentSegments,
    const uint enableParticleFieldContributions,
    int nTracers,
    int tracerLength)
{
    int id = get_global_id(0);
    if (id >= nTracers) return;

    int traceStart = id * tracerLength;
    float3 loc = bTracerTrails[traceStart];

    for (int i = 1; i < tracerLength; i++) {
        // Calculate the B field at location
        float3 E = (float3)(0.0f, 0.0f, 0.0f);
        float3 B = (float3)(0.0f, 0.0f, 0.0f);

        if (enableParticleFieldContributions) {
            int unused = -1;
            compute_particle_field_contributions(nParticles, particlePos, particleVel, loc, -1, &E, &B, &unused);
        }

        compute_current_field_contributions(currentSegments, nCurrentSegments, loc, &B);

        loc += normalize(B) * 0.005f * _M;
        bTracerTrails[traceStart + i] = loc;
    }
}
