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

__kernel void computeMotion(
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    __global float4* debug,
    const float dt,
    const uint nParticles,
    const uint nCurrentSegments,
    const float solenoidFlux,
    const uint calcInterparticlePhysics)
{
    int id = get_global_id(0);
    if (id >= nParticles) return;

    // Extract position and charge-to-mass ratio for this particle
    float3 pos = (float3)(particlePos[id][0], particlePos[id][1], particlePos[id][2]);
    float3 vel = (float3)(particleVel[id][0], particleVel[id][1], particleVel[id][2]);
    float type = particlePos[id][3];
    float q_over_m = 0.0;

    if      (type == -1.0) q_over_m = Q_OVER_M_ELECTRON;
    else if (type ==  1.0) q_over_m = Q_OVER_M_PROTON;
    else if (type ==  0.0) q_over_m = 0.0;

    // Calculate the E and B field at particle position
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);

    if (calcInterparticlePhysics) {
        for (int j = 0; j < nParticles; j++) {
            if (j == id) continue;

            float3 otherPos = (float3)(particlePos[j][0], particlePos[j][1], particlePos[j][2]);
            float3 otherVel = (float3)(particleVel[j][0], particleVel[j][1], particleVel[j][2]);
            float otherType = particlePos[j][3];
            float otherCharge = 0.0;

            if      (otherType == -1.0) otherCharge = -Q; // electron
            else if (otherType ==  1.0) otherCharge = Q;  // proton
            else if (otherType ==  0.0) otherCharge = 0;  // neutron

            float3 r = pos - otherPos;
            float3 r_norm = normalize(r);
            float r_mag = length(r);

            // Avoid division by zero
            if (r_mag < 0.00001f) continue;

            float3 e = ((K * otherCharge) / (r_mag * r_mag)) * r_norm;
            float3 b = ((MU_0_OVER_4_PI * otherCharge) / (r_mag * r_mag)) * cross(otherVel, r_norm);

            E += e;
            B += b;
        }
    }

    for (int j = 0; j < nCurrentSegments; j++) {
        float3 current_x = (float3)(currentSegments[j*3][0], currentSegments[j*3][1], currentSegments[j*3][2]);
        float3 current_dx = (float3)(currentSegments[j*3 + 1][0], currentSegments[j*3 + 1][1], currentSegments[j*3 + 1][2]);
        float current_i = currentSegments[j*3 + 2][0];

        float3 r = pos - current_x;
        float r_mag = length(r);

        B += MU_0_OVER_4_PI * current_i * cross(current_dx, r) / (r_mag * r_mag * r_mag);
    }

    // Calculate the contribution of the central solenoid
    float3 solenoid_axis = (float3)(0.0, 1.0, 0.0);
    float3 solenoid_r = (float3)(pos[0], 0.0, pos[2]);
    float solenoid_e_mag = solenoidFlux / (2.0 * PI * length(solenoid_r));
    E += solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));

    // Push the particle through the electric and magnetic field: dv/dt = q/m (E + v x B);
    float3 t = q_over_m * B * 0.5f * dt;
    float3 s = 2.0f * t / (1.0f + (length(t) * length(t)));
    float3 v_minus = vel + q_over_m * E * 0.5f * dt;
    float3 v_prime = v_minus + cross(v_minus, t);
    float3 v_plus = v_minus + cross(v_prime, s);
    float3 vel_new = v_plus + (q_over_m * E * 0.5f * dt);
    float3 pos_new = pos + (vel_new * dt);

    particlePos[id] = (float4)(pos_new[0], pos_new[1], pos_new[2], type);
    particleVel[id] = (float4)(vel_new[0], vel_new[1], vel_new[2], 0.0);

    // Keep the particles in their box
    if (particlePos[id][0] >  2.0) particleVel[id][0] = -particleVel[id][0];
    if (particlePos[id][0] < -2.0) particleVel[id][0] = -particleVel[id][0];
    if (particlePos[id][1] >  2.0) particleVel[id][1] = -particleVel[id][1];
    if (particlePos[id][1] < -2.0) particleVel[id][1] = -particleVel[id][1];
    if (particlePos[id][2] >  2.0) particleVel[id][2] = -particleVel[id][2];
    if (particlePos[id][2] < -2.0) particleVel[id][2] = -particleVel[id][2];
}
