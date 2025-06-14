__kernel void updateTrails(
    __global uint* nParticles,
    __global float3* tracerTrails, 
    __global float4* eField,
    __global float4* bField,
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    const uint nCurrentSegments,
    const float solenoidFlux,
    const uint enableInterparticlePhysics,
    int nTracers,
    int tracerLength)
{
    int id = get_global_id(0);
    if (id >= nTracers) return;

    int traceStart = id * tracerLength;
    float3 loc = tracerTrails[traceStart];

    for (int i = 1; i < tracerLength; i++) {
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
                float charge = particle_charge(species);

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

        loc += normalize(B) * 0.005f * _M;
        tracerTrails[traceStart + i] = loc;
    }
}
