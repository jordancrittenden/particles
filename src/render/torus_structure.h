#pragma once

#include <webgpu/webgpu_cpp.h>
#include "util/wgpu_util.h"

typedef struct TorusStructureBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    std::vector<unsigned int> indices;
} TorusStructureBuffers;

TorusStructureBuffers create_torus_structure_buffers(wgpu::Device& device, float r1, float r2, int toroidalSegments, int poloidalSegments);

void render_torus_structure(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusStructureBuffers& torusStructureBuf, glm::mat4 view, glm::mat4 projection); 