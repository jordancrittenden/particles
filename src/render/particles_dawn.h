#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>

struct ParticleBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
};

ParticleBuffers create_particle_buffers(wgpu::Device& device, const std::vector<glm::vec4>& positions);

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, int nParticles, glm::mat4 view, glm::mat4 projection);
