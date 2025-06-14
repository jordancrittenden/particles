__kernel void computeFields(
    __global uint* nParticles,
    __global float4* cellLocation,
    __global float4* eField,
    __global float4* bField,
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    __global float4* debug,
    const uint nCells,
    const uint nCurrentSegments,
    const float solenoidFlux,
    const uint enableParticleFieldContributions)
{
    int id = get_global_id(0);
    if (id >= nCells) return;

    // Extract position for this cell
    float3 loc = (float3)(cellLocation[id][0], cellLocation[id][1], cellLocation[id][2]);

    // Calculate the E and B field at location
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);

    if (enableParticleFieldContributions) {
        int unused = -1;
        compute_particle_field_contributions(nParticles, particlePos, particleVel, loc, -1, &E, &B, &unused);
    }

    for (int j = 0; j < nCurrentSegments; j++) {
        float3 current_x = (float3)(currentSegments[j*3][0], currentSegments[j*3][1], currentSegments[j*3][2]);
        float3 current_dx = (float3)(currentSegments[j*3 + 1][0], currentSegments[j*3 + 1][1], currentSegments[j*3 + 1][2]);
        float current_i = currentSegments[j*3 + 2][0];

        float3 r = loc - current_x;
        float r_mag = length(r);

        B += MU_0_OVER_4_PI * current_i * cross(current_dx, r) / (r_mag * r_mag * r_mag);
    }

    // Calculate the contribution of the central solenoid
    float3 solenoid_axis = (float3)(0.0, 1.0, 0.0);
    float3 solenoid_r = (float3)(loc[0], 0.0, loc[2]);
    float solenoid_e_mag = solenoidFlux / (2.0 * PI * length(solenoid_r));
    E += solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));

    eField[id] = (float4)(E[0], E[1], E[2], 0.0);
    bField[id] = (float4)(B[0], B[1], B[2], 0.0);

    debug[id] = (float4)(loc[0], loc[1], loc[2], 0.0);
}
