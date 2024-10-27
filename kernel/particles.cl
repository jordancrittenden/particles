#define M_ELECTRON (9.10938188e-31) /* kg */
#define M_PROTON   (1.67262158e-27) /* kg */
#define M_NEUTRON  (1.67492716e-27) /* kg */
#define Q          (1.602176487e-19) /* A s */

__kernel void computeMotion(
    __global float4* positions,
    __global float4* velocities, 
    const float dt,
    const int N)
{
    int id = get_global_id(0);
    if (id >= N) return;

    // Extract position, charge, mass for this particle
    float3 pos = (float3)(positions[id][0], positions[id][1], positions[id][2]);
    float3 vel = (float3)(velocities[id][0], velocities[id][1], velocities[id][2]);
    float type = positions[id][3];
    float charge = 0.0;
    float mass = 0.0;

    if (type == -1.0) {
        // electron
        charge = -Q;
        mass = M_ELECTRON;
    } else if (type == 1.0) {
        // proton
        charge = Q;
        mass = M_PROTON;
    } else if (type == 0.0) {
        // neutron
        charge = 0;
        mass = M_NEUTRON;
    }

    // Calculate the E and B field at particle position
    float3 E = (float3)(0.0f, 0.0f, 0.0f);
    float3 B = (float3)(0.0f, 0.0f, 0.0f);
    for (int j = 0; j < N; j++) {
        if (j == id) continue;

        float3 otherPos = (positions[j][0], positions[j][1], positions[j][2]);
        float otherCharge = positions[j][3];

        float3 r = otherPos - pos;
        float r_norm = length(r);

        // Avoid division by zero
        if (r_norm > 0.001f) {
            
        }
    }

    // Push the particle through the electric and magnetic field: dv/dt = q/m (E + v x B);
    float q2m = charge / mass;
    float3 t = q2m * B * 0.5f * dt;
    float3 s = 2.0f * t / (1.0f + length(t) * length(t));
    float3 v_minus = vel + q2m * E * 0.5f * dt;
    float3 v_prime = v_minus + cross(v_minus, t);
    float3 v_plus = v_minus + cross(v_prime, s);
    float3 vel_new = v_plus + q2m * E * 0.5f * dt;
    float3 pos_new = pos + (vel_new * dt);

    positions[id]  = (float4)(pos_new[0], pos_new[1], pos_new[2], type);
    velocities[id] = (float4)(vel_new[0], vel_new[1], vel_new[2], 0.0);

    // Keep the particles in their box
    if (positions[id][0] >  5.0) velocities[id][0] = -velocities[id][0];
    if (positions[id][0] < -5.0) velocities[id][0] = -velocities[id][0];
    if (positions[id][1] >  5.0) velocities[id][1] = -velocities[id][1];
    if (positions[id][1] < -5.0) velocities[id][1] = -velocities[id][1];
    if (positions[id][2] >  5.0) velocities[id][2] = -velocities[id][2];
    if (positions[id][2] < -5.0) velocities[id][2] = -velocities[id][2];
}
