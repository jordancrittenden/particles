#pragma once

#include <dawn/webgpu_cpp.h>
#include "../util/wgpu_util.h"
#include "ring_dawn.h"

struct TorusBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer instanceBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    std::vector<unsigned int> indices;
    int numRings;
};

TorusBuffers create_torus_buffers(wgpu::Device& device, const Ring& ring, int numRings);

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusBuf, glm::mat4 view, glm::mat4 projection); 