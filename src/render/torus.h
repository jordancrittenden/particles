#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "ring.h"

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
    glm::u16 nCoils;
};

glm::mat4 get_coil_model_matrix(glm::f32 angle, glm::f32 r1);

TorusBuffers create_torus_buffers(wgpu::Device& device, const Ring& ring, glm::u16 nCoils);

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusBuf, glm::f32 primaryRadius, glm::f32 toroidalI, glm::mat4 view, glm::mat4 projection); 