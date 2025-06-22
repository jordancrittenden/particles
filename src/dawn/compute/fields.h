#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "shared/particles.h"
#include "shared/fields.h"
#include "cell.h"

struct FieldCompute {
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;

    wgpu::Buffer cellLocationBuffer;
    wgpu::Buffer debugBuffer;
    wgpu::Buffer paramsBuffer;
};

FieldCompute create_field_compute(
    wgpu::Device& device,
    const std::vector<Cell>& cells,
    const ParticleBuffers& particleBuf,
    const FieldBuffers& fieldBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles);

void run_field_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& pass,
    FieldCompute& fieldCompute,
    glm::u32 nCells,
    glm::u32 nCurrentSegments,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions); 