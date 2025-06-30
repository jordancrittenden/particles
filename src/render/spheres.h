#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "shared/particles.h"
#include "physical_constants.h"

struct SphereRender {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    glm::u32 indexCount;
};

SphereRender create_sphere_render(wgpu::Device& device);

void render_particles_as_spheres(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const ParticleBuffers& particleBuf,
    const SphereRender& render,
    glm::u32 nParticles,
    glm::mat4 view,
    glm::mat4 projection);