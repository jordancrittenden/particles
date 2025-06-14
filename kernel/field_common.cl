void compute_particle_field_contributions(
    __global uint* nParticles,
    __global float4* particlePos,
    __global float4* particleVel,
    float3 loc,
    int skipId,
    float3* E,
    float3* B,
    int* colliderId)
{
    for (int i = 0; i < *nParticles; i++) {
        if (i == skipId) continue;

        float species = particlePos[i][3];
        if (species == 0.0) continue; // inactive particle

        float3 pos = (float3)(particlePos[i][0], particlePos[i][1], particlePos[i][2]);
        float3 vel = (float3)(particleVel[i][0], particleVel[i][1], particleVel[i][2]);
        float charge = particle_charge(species);

        float3 r = loc - pos;
        float3 r_norm = normalize(r);
        float r_mag = length(r);

        // Avoid division by zero
        if (r_mag < 0.00001f) {
            *colliderId = i;
            continue;
        } else {
            *E += ((K * charge) / (r_mag * r_mag)) * r_norm;
            *B += ((MU_0_OVER_4_PI * charge) / (r_mag * r_mag)) * cross(vel, r_norm);
        }
    }
}