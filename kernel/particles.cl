__kernel void computeMotion(__global float4* positions, __global float4* velocities, 
                            __global int* types, const float deltaTime, const int numParticles) {
    int id = get_global_id(0);
    if (id >= numParticles) return;

    // Initialize force vector
    float4 force = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

    for (int j = 0; j < numParticles; j++) {
        if (j == id) continue;

        float4 diff = positions[j] - positions[id];
        float distance = length(diff);

        // Avoid division by zero
        if (distance > 0.001f) {
            float charge = (types[id] == types[j]) ? 1.0f : -1.0f; // Repel if same type, attract otherwise
            float forceMagnitude = charge / (distance * distance);
            force += (forceMagnitude / distance) * diff;
        }
    }

    // Update velocity and position
    velocities[id] += force * deltaTime;
    positions[id] += velocities[id] * deltaTime;
}
