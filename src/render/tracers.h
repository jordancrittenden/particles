#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "shared/tracers.h"
#include "physical_constants.h"

struct TracerRender {
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    glm::u32 headIdx;
};

TracerRender create_tracer_render(wgpu::Device& device);

void render_e_tracers(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const TracerBuffers& tracerBuf,
    TracerRender& render,
    glm::mat4 view,
    glm::mat4 projection);

void render_b_tracers(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const TracerBuffers& tracerBuf,
    TracerRender& render,
    glm::mat4 view,
    glm::mat4 projection);