__kernel void defrag(
    __global uint* nParticles,
    __global float4* particlePos,
    __global float4* particleVel,
    uint maxParticles)
{
    int write_head = 0;
    for (int i = 0; i < maxParticles; i++) {
        float species = particlePos[i][3];
        if (species == 0.0) continue;

        if (write_head < i) {
            particlePos[write_head] = particlePos[i];
            particleVel[write_head] = particleVel[i];
            particlePos[i] = (float4)(0.0, 0.0, 0.0, 0.0);
            particleVel[i] = (float4)(0.0, 0.0, 0.0, 0.0);
        }

        write_head++;
    }

    *nParticles = write_head;
}