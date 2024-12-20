#define PI                    (3.14159265953f)

#define M_ELECTRON            (9.10938188e-31f)                   /* kg */
#define M_PROTON              (1.67262158e-27f)                   /* kg */
#define M_NEUTRON             (1.67492716e-27f)                   /* kg */
#define M_DEUTERIUM           (3.34449439e-27f)                   /* kg */
#define M_TRITIUM             (5.00826721e-27f)                   /* kg */
#define M_HELIUM_4_NUC        (6.64647309e-27f)                   /* kg */
#define M_DEUTERON            (3.34358347e-27f)                   /* kg */
#define M_TRITON              (5.00735629e-27f)                   /* kg */

#define EPSILON_0             (8.854187817e-12f)                  /* A^2 s^4 / kg m^3 */
#define MU_0                  (1.25663706144e-6f)                 /* kg m / A^2 s^2 */
#define Q                     (1.602176487e-19f)                  /* A s */
#define K                     (1.0f / (4.0f * PI * EPSILON_0))    /* kg m^3 / A^2 s^4 */
#define MU_0_OVER_4_PI        (MU_0 / (4.0f * PI))                /* kg m / A^2 s^2 */

#define Q_OVER_M_ELECTRON     (-1.75882020109e11f)                /* A s / kg */
#define Q_OVER_M_PROTON       ( 9.57883424534e7f)                 /* A s / kg */
#define Q_OVER_M_HELIUM_4_NUC ( 2.41056642418e7f)                 /* A s / kg */
#define Q_OVER_M_DEUTERON     ( 4.79179449646e7f)                 /* A s / kg */
#define Q_OVER_M_TRITON       ( 3.19964547001e7f)                 /* A s / kg */

#define CONSTRAIN_TO          0.1                                /* m */

float particle_mass(float species) {
    if      (species == 1.0) return M_NEUTRON;
    else if (species == 2.0) return M_ELECTRON;
    else if (species == 3.0) return M_PROTON;
    else if (species == 4.0) return M_DEUTERIUM;
    else if (species == 5.0) return M_TRITIUM;
    else if (species == 6.0) return M_HELIUM_4_NUC;
    else if (species == 7.0) return M_DEUTERON;
    else if (species == 8.0) return M_TRITON;
}

float particle_charge(float species) {
    if      (species == 1.0) return 0;       // neutron
    else if (species == 2.0) return -Q;      // electron
    else if (species == 3.0) return Q;       // proton
    else if (species == 4.0) return 0;       // deuterium
    else if (species == 5.0) return 0;       // tritium
    else if (species == 6.0) return 2.0 * Q; // helium4
    else if (species == 7.0) return Q;       // ion_deuterium
    else if (species == 8.0) return Q;       // ion_tritium
}

float charge_to_mass_ratio(float species) {
    if      (species == 1.0) return 0.0;
    else if (species == 2.0) return Q_OVER_M_ELECTRON;
    else if (species == 3.0) return Q_OVER_M_PROTON;
    else if (species == 4.0) return 0;
    else if (species == 5.0) return 0;
    else if (species == 6.0) return Q_OVER_M_HELIUM_4_NUC;
    else if (species == 7.0) return Q_OVER_M_DEUTERON;
    else if (species == 8.0) return Q_OVER_M_TRITON;
}

__kernel void computeMotion(
    __global uint* nParticles,
    __global float4* particlePos,
    __global float4* particleVel,
    __global float4* currentSegments,
    __global float4* debug,
    const float dt,
    const uint nCurrentSegments,
    const float solenoidFlux,
    const uint enableInterparticlePhysics)
{
    int id = get_global_id(0);
    if (id >= *nParticles) return;

    float species = particlePos[id][3];
    if (species == 0.0) return; // inactive particle

    // Extract position and charge-to-mass ratio for this particle
    float3 pos = (float3)(particlePos[id][0], particlePos[id][1], particlePos[id][2]);
    float3 vel = (float3)(particleVel[id][0], particleVel[id][1], particleVel[id][2]);
    float mass = particle_mass(species);
    float q_over_m = charge_to_mass_ratio(species);

    // Calculate the E and B field at particle position
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);

    if (enableInterparticlePhysics) {
        int collider_id = -1;
        float collider_mass;
        float3 collider_pos;
        float3 collider_vel;
        float3 collision_r_norm;
        for (int j = 0; j < *nParticles; j++) {
            if (j == id) continue;

            float otherSpecies = particlePos[j][3];
            if (otherSpecies == 0.0) continue;

            float3 otherPos = (float3)(particlePos[j][0], particlePos[j][1], particlePos[j][2]);
            float3 otherVel = (float3)(particleVel[j][0], particleVel[j][1], particleVel[j][2]);
            float otherCharge = particle_charge(otherSpecies);

            float3 r = pos - otherPos;
            float3 r_norm = normalize(r);
            float r_mag = length(r);

            // Check for collisions
            if (r_mag < 0.00001f) {
                collider_id = j;
                collider_mass = particle_mass(otherSpecies);
                collider_pos = otherPos;
                collider_vel = otherVel;
                collision_r_norm = r_norm;
            } else {
                E += ((K * otherCharge) / (r_mag * r_mag)) * r_norm;
                B += ((MU_0_OVER_4_PI * otherCharge) / (r_mag * r_mag)) * cross(otherVel, r_norm);
            }
        }
        // to avoid both work items spawning a new particle, check id < collider_id, which will only be true for one of them
        if (collider_id >= 0 && id < collider_id) {
            // Relative velocity
            float3 v = collider_vel - vel;

            // Relative velocity along the line of collision
            float v_dot_r = dot(v, collision_r_norm);

            // If the relative velocity along the line of collision is >= 0, they are moving apart
            if (v_dot_r >= 0.0) return;

            // Impulse scalar
            float impulse = (2.0 * v_dot_r) / (mass + collider_mass);

            // Compute new velocities
            float3 vel_new = vel + impulse * mass * collision_r_norm;
            float3 collider_vel_new = collider_vel - impulse * mass * collision_r_norm;

            particleVel[id] = (float4)(vel_new.x, vel_new.y, vel_new.z, 0.0f);
            particleVel[collider_id] = (float4)(collider_vel_new.x, collider_vel_new.y, collider_vel_new.z, 0.0f);

            // Create new particle
            uint new_idx = *nParticles + id;
            particlePos[new_idx] = (float4)(pos[0] + 0.01, pos[1] + 0.01, pos[2] + 0.01, 1.0);
            particleVel[new_idx] = (float4)(0.0, 10000.0, 0.0, 0.0);
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

    particlePos[id] = (float4)(pos_new[0], pos_new[1], pos_new[2], species);
    particleVel[id] = (float4)(vel_new[0], vel_new[1], vel_new[2], 0.0);

    float v_mag = length(vel_new);
    debug[id] = (float4)(0.5 * mass * v_mag * v_mag, species, 0.0, 0.0);

    // Keep the particles in their box
    if (particlePos[id][0] >  CONSTRAIN_TO) particleVel[id][0] = -particleVel[id][0];
    if (particlePos[id][0] < -CONSTRAIN_TO) particleVel[id][0] = -particleVel[id][0];
    if (particlePos[id][1] >  CONSTRAIN_TO) particleVel[id][1] = -particleVel[id][1];
    if (particlePos[id][1] < -CONSTRAIN_TO) particleVel[id][1] = -particleVel[id][1];
    if (particlePos[id][2] >  CONSTRAIN_TO) particleVel[id][2] = -particleVel[id][2];
    if (particlePos[id][2] < -CONSTRAIN_TO) particleVel[id][2] = -particleVel[id][2];
}
