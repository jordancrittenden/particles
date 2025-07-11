#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "util/wgpu_util.h"
#include "shared/particles.h"

struct TorusWallCompute {
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::Buffer paramsBuffer;
};

TorusWallCompute create_torus_wall_compute(
    wgpu::Device& device,
    const ParticleBuffers& particleBuf,
    glm::u32 maxParticles);

void run_torus_wall_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const TorusWallCompute& torusWallCompute,
    glm::f32 r1,
    glm::f32 r2,
    glm::u32 nParticles);
