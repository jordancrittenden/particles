#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

struct FieldRender {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer instanceBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
};

FieldRender create_fields_render(wgpu::Device& device, std::vector<glm::f32vec4>& loc, std::vector<glm::f32vec4>& vec, float length);

void render_fields(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const FieldRender& fieldRender,
    wgpu::Buffer& fieldBuffer,
    int numFieldVectors,
    glm::mat4 view,
    glm::mat4 projection);