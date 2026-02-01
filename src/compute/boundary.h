#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "util/wgpu_util.h"
#include "shared/particles.h"

struct BoundaryCompute {
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::Buffer paramsBuffer;
};

BoundaryCompute create_boundary_compute(
    wgpu::Device& device,
    const ParticleBuffers& particleBuf,
    glm::u32 maxParticles);

void run_boundary_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const BoundaryCompute& boundaryCompute,
    glm::f32 x_min,
    glm::f32 x_max,
    glm::f32 y_min,
    glm::f32 y_max,
    glm::f32 z_min,
    glm::f32 z_max,
    glm::u32 nParticles);
