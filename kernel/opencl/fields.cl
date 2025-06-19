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

    compute_current_field_contributions(currentSegments, nCurrentSegments, loc, &B);

    // Calculate the contribution of the central solenoid
    float3 solenoid_axis = (float3)(0.0, 1.0, 0.0);
    float3 solenoid_r = (float3)(loc[0], 0.0, loc[2]);
    float solenoid_e_mag = solenoidFlux / (2.0 * PI * length(solenoid_r));
    E += solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));

    eField[id] = (float4)(E[0], E[1], E[2], 0.0);
    bField[id] = (float4)(B[0], B[1], B[2], 0.0);

    debug[id] = (float4)(loc[0], loc[1], loc[2], 0.0);
}
