#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "shared/tracers.h"
#include "shared/particles.h"

struct TracerCompute {
    wgpu::ComputePipeline ePipeline;
    wgpu::ComputePipeline bPipeline;
    wgpu::BindGroup eBindGroup;
    wgpu::BindGroup bBindGroup;
    wgpu::BindGroupLayout eBindGroupLayout;
    wgpu::BindGroupLayout bBindGroupLayout;

    wgpu::Buffer eParamsBuffer;
    wgpu::Buffer bParamsBuffer;

    // Debug buffers
    wgpu::Buffer eDebugStorageBuf;
    wgpu::Buffer eDebugReadBuf;
    wgpu::Buffer bDebugStorageBuf;
    wgpu::Buffer bDebugReadBuf;

    glm::u32 curTraceIdxE;
    glm::u32 curTraceIdxB;
};

TracerCompute create_tracer_compute(
    wgpu::Device& device,
    const TracerBuffers& tracerBuf,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles);

void run_tracer_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    TracerCompute& compute,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments,
    glm::u32 nParticles,
    glm::u32 nTracers,
    glm::u32 tracerLength);

void read_e_tracer_debug(wgpu::Device& device, wgpu::Instance& instance, const TracerCompute& compute, std::vector<glm::f32vec4>& debug, glm::u32 n);

void read_b_tracer_debug(wgpu::Device& device, wgpu::Instance& instance, const TracerCompute& compute, std::vector<glm::f32vec4>& debug, glm::u32 n);