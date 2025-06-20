#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "physical_constants.h"

struct ParticleBuffers {
    // Shared buffers (used by both compute and render)
    wgpu::Buffer sharedPosBuffer;      // Shared position buffer
    wgpu::Buffer sharedVelBuffer;      // Shared velocity buffer
    
    // Render-specific buffers
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    
    // Compute-specific buffers
    wgpu::ComputePipeline computePipeline;
    wgpu::BindGroup computeBindGroup;
    wgpu::BindGroupLayout computeBindGroupLayout;
    
    // Persistent compute buffers (reused every frame)
    wgpu::Buffer nParticlesBuffer;        // Current number of particles
    wgpu::Buffer currentSegmentsBuffer;   // Current segments data
    wgpu::Buffer debugBuffer;             // Debug output buffer
    wgpu::Buffer paramsBuffer;            // Compute parameters (dt, etc.)
};

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::f32vec4()> posF,
    std::function<glm::f32vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    glm::u32 initialParticles,
    glm::u32 maxParticles);

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, int nParticles, glm::mat4 view, glm::mat4 projection);

void run_particle_compute(wgpu::Device& device, wgpu::ComputePassEncoder& computePass, const ParticleBuffers& particleBuf, glm::u32 nParticles, glm::f32 dt, float solenoidFlux, glm::u32 enableParticleFieldContributions);
