#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
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
    int nCoils;
};

glm::mat4 get_coil_model_matrix(float angle, float r1);

TorusBuffers create_torus_buffers(wgpu::Device& device, const Ring& ring, int nCoils);

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusBuf, float primaryRadius, glm::mat4 view, glm::mat4 projection); 