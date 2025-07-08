#pragma once

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include "mesh.h"

struct CellBoxBuffers {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer instanceBuffer;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    std::vector<unsigned int> indices;
    glm::u32 nCells;
    glm::u32 nVisibleCells;
};

CellBoxBuffers create_cell_box_buffers(wgpu::Device& device, const std::vector<Cell>& cells, glm::f32 dx);

void render_cell_boxes(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const CellBoxBuffers& cellBoxBuf, glm::mat4 view, glm::mat4 projection);

void update_cell_visibility(wgpu::Device& device, CellBoxBuffers& cellBoxBuf, const std::vector<Cell>& cells, const std::vector<bool>& visibility);
