#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include "physical_constants.h"

struct ParticleBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
};

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::vec4()> posF,
    std::function<glm::vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    int initialParticles,
    int maxParticles);

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, int nParticles, glm::mat4 view, glm::mat4 projection);
