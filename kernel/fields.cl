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
    const uint enableInterparticlePhysics)
{
    int id = get_global_id(0);
    if (id >= nCells) return;

    // Extract position and charge-to-mass ratio for this particle
    float3 loc = (float3)(cellLocation[id][0], cellLocation[id][1], cellLocation[id][2]);

    // Calculate the E and B field at location
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);

    if (enableInterparticlePhysics) {
        for (int j = 0; j < *nParticles; j++) {
            if (j == id) continue;

            float species = particlePos[j][3];
            if (species == 0.0) continue; // inactive particle

            float3 pos = (float3)(particlePos[j][0], particlePos[j][1], particlePos[j][2]);
            float3 vel = (float3)(particleVel[j][0], particleVel[j][1], particleVel[j][2]);
            float charge = 0.0;

            if      (species == 1.0) charge = 0;       // neutron
            else if (species == 2.0) charge = -Q;      // electron
            else if (species == 3.0) charge = Q;       // proton
            else if (species == 4.0) charge = 0;       // deuterium
            else if (species == 5.0) charge = 0;       // tritium
            else if (species == 6.0) charge = 2.0 * Q; // helium4
            else if (species == 7.0) charge = Q;       // ion_deuterium
            else if (species == 8.0) charge = Q;       // ion_tritium

            float3 r = loc - pos;
            float3 r_norm = normalize(r);
            float r_mag = length(r);

            // Avoid division by zero
            if (r_mag < 0.00001f) continue;

            float3 e = ((K * charge) / (r_mag * r_mag)) * r_norm;
            float3 b = ((MU_0_OVER_4_PI * charge) / (r_mag * r_mag)) * cross(vel, r_norm);

            E += ((K * charge) / (r_mag * r_mag)) * r_norm;
            B += ((MU_0_OVER_4_PI * charge) / (r_mag * r_mag)) * cross(vel, r_norm);
        }
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
