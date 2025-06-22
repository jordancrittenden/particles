#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>

struct AxesBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
};

AxesBuffers create_axes_buffers(wgpu::Device& device);

void render_axes(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const AxesBuffers& axesBuf, glm::mat4 view, glm::mat4 projection);
