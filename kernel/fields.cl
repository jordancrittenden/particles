#define PI                (3.14159265953f)
#define M_ELECTRON        (9.10938188e-31f)                   /* kg */
#define M_PROTON          (1.67262158e-27f)                   /* kg */
#define M_NEUTRON         (1.67492716e-27f)                   /* kg */
#define EPSILON_0         (8.854187817e-12f)                  /* A^2 s^4 / kg m^3 */
#define MU_0              (1.25663706144e-6f)                 /* kg m / A^2 s^2 */
#define Q                 (1.602176487e-19f)                  /* A s */
#define K                 (1.0f / (4.0f * PI * EPSILON_0))    /* kg m^3 / A^2 s^4 */
#define MU_0_OVER_4_PI    (MU_0 / (4.0f * PI))                /* kg m / A^2 s^2 */

#define Q_OVER_M_ELECTRON (-1.75882020109e11f)                /* A s / kg */
#define Q_OVER_M_PROTON   ( 9.57883424534e7f)                 /* A s / kg */

__kernel void computeFields(
    __global float4* cellLocation,
    __global float4* eField,
    __global float4* bField,
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    __global float4* debug,
    const uint nCells,
    const uint nParticles,
    const uint nCurrentSegments,
    const float solenoidE0,
    const uint calcInterparticlePhysics)
{
    int id = get_global_id(0);
    if (id >= nCells) return;

    // Extract position and charge-to-mass ratio for this particle
    float3 loc = (float3)(cellLocation[id][0], cellLocation[id][1], cellLocation[id][2]);

    // Calculate the E and B field at location
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);

    if (calcInterparticlePhysics) {
        for (int j = 0; j < nParticles; j++) {
            if (j == id) continue;

            float3 pos = (float3)(particlePos[j][0], particlePos[j][1], particlePos[j][2]);
            float3 vel = (float3)(particleVel[j][0], particleVel[j][1], particleVel[j][2]);
            float type = particlePos[j][3];
            float charge = 0.0;

            if      (type == -1.0) charge = -Q; // electron
            else if (type ==  1.0) charge = Q;  // proton
            else if (type ==  0.0) charge = 0;  // neutron

            float3 r = loc - pos;
            float3 r_norm = normalize(r);
            float r_mag = length(r);

            // Avoid division by zero
            if (r_mag < 0.00001f) continue;

            float3 e = ((K * charge) / (r_mag * r_mag)) * r_norm;
            float3 b = ((MU_0_OVER_4_PI * charge) / (r_mag * r_mag)) * cross(vel, r_norm);

            E += e;
            B += b;
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
    float3 solenoid_r_norm = normalize(solenoid_r);
    float solenoid_e_mag = solenoidE0 / length(solenoid_r);
    float3 solenoid_e = solenoid_e_mag * cross(solenoid_axis, solenoid_r_norm);
    E += solenoid_e;

    eField[id] = (float4)(E, 0.0);
    bField[id] = (float4)(B, 0.0);

    debug[id] = (float4)(loc[0], loc[1], loc[2], 0.0);
}
