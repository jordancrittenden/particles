#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "shared/particles.h"

struct ParticleCompute {
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;

    wgpu::Buffer nParticlesBuffer;
    wgpu::Buffer debugBuffer;
    wgpu::Buffer paramsBuffer;
};

ParticleCompute create_particle_compute(
    wgpu::Device& device,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles);

void run_particle_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& compute,
    glm::u32 nParticles,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments);