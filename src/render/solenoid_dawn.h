#pragma once

#include <dawn/webgpu_cpp.h>
#include "../util/wgpu_util.h"
#include "ring_dawn.h"

typedef struct SolenoidBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    std::vector<unsigned int> indices;
} SolenoidBuffers;

SolenoidBuffers create_solenoid_buffers(wgpu::Device& device, const Ring& ring);

void render_solenoid(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const SolenoidBuffers& solenoidBuf, glm::mat4 view, glm::mat4 projection); 