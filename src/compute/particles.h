#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "shared/particles.h"
#include "shared/fields.h"
#include "mesh.h"

struct ParticleCompute {
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;

    wgpu::Buffer nParticlesReadBuf;
    wgpu::Buffer debugStorageBuf;
    wgpu::Buffer debugReadBuf;
    wgpu::Buffer paramsBuffer;
    wgpu::Buffer meshBuffer;
    wgpu::Buffer cellLocationBuffer;
};

ParticleCompute create_particle_compute(
    wgpu::Device& device,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles);

ParticleCompute create_particle_pic_compute(
    wgpu::Device& device,
    const std::vector<Cell>& cells,
    const ParticleBuffers& particleBuf,
    const FieldBuffers& fieldBuf,
    glm::u32 maxParticles);

void run_particle_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& compute,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments,
    glm::u32 nParticles);

void run_particle_pic_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& particleCompute,
    const MeshProperties& mesh,
    glm::f32 dt,
    glm::u32 nParticles);

glm::u32 read_nparticles(wgpu::Device& device, wgpu::Instance& instance, const ParticleCompute& compute);

void read_particles_debug(wgpu::Device& device, wgpu::Instance& instance, const ParticleCompute& compute, std::vector<glm::f32vec4>& debug, glm::u32 n);