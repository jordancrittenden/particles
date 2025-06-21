#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "shared/particles.h"
#include "physical_constants.h"

struct ParticleRender {
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
};

ParticleRender create_particle_render(wgpu::Device& device);

void render_particles(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const ParticleBuffers& particleBuf,
    const ParticleRender& render,
    glm::u32 nParticles,
    glm::mat4 view,
    glm::mat4 projection);