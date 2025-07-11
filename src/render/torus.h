#pragma once

#include <webgpu/webgpu_cpp.h>
#include "util/wgpu_util.h"

struct TorusBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    std::vector<unsigned int> indices;
};

TorusBuffers create_torus_buffers(wgpu::Device& device, float r1, float r2, int toroidalSegments, int poloidalSegments);

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusStructureBuf, glm::mat4 view, glm::mat4 projection); 