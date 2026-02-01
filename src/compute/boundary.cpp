#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/boundary.h"

struct BoundaryParams {
    glm::f32 x_min;
    glm::f32 x_max;
    glm::f32 y_min;
    glm::f32 y_max;
    glm::f32 z_min;
    glm::f32 z_max;
};

BoundaryCompute create_boundary_compute(wgpu::Device& device, const ParticleBuffers& particleBuf, glm::u32 maxParticles) {
    BoundaryCompute boundaryCompute = {};

    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/boundary.wgsl");
    if (!computeShaderModule) {
        std::cerr << "Failed to create boundary compute shader module" << std::endl;
        exit(1);
    }

    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Boundary Compute Params Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(BoundaryParams),
        .mappedAtCreation = false
    };
    boundaryCompute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        {
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(glm::u32)
            }
        }, {
            .binding = 1,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, {
            .binding = 2,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, {
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(BoundaryParams)
            }
        }
    };

    wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc = {
        .label = "Boundary Compute Bind Group Layout",
        .entryCount = static_cast<uint32_t>(computeBindings.size()),
        .entries = computeBindings.data()
    };
    boundaryCompute.bindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Boundary Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &boundaryCompute.bindGroupLayout
    };
    wgpu::PipelineLayout computePipelineLayout = device.CreatePipelineLayout(&computePipelineLayoutDesc);

    wgpu::ComputePipelineDescriptor computePipelineDesc = {
        .label = "Boundary Compute Pipeline",
        .layout = computePipelineLayout,
        .compute = {
            .module = computeShaderModule,
            .entryPoint = "applyBoundary"
        }
    };
    boundaryCompute.pipeline = device.CreateComputePipeline(&computePipelineDesc);

    std::vector<wgpu::BindGroupEntry> computeEntries = {
        {
            .binding = 0,
            .buffer = particleBuf.nCur,
            .offset = 0,
            .size = sizeof(uint32_t)
        }, {
            .binding = 1,
            .buffer = particleBuf.pos,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, {
            .binding = 2,
            .buffer = particleBuf.vel,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, {
            .binding = 3,
            .buffer = boundaryCompute.paramsBuffer,
            .offset = 0,
            .size = sizeof(BoundaryParams)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Boundary Compute Bind Group",
        .layout = boundaryCompute.bindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    boundaryCompute.bindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return boundaryCompute;
}

void run_boundary_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const BoundaryCompute& boundaryCompute,
    glm::f32 x_min,
    glm::f32 x_max,
    glm::f32 y_min,
    glm::f32 y_max,
    glm::f32 z_min,
    glm::f32 z_max,
    glm::u32 nParticles)
{
    BoundaryParams params = {
        .x_min = x_min,
        .x_max = x_max,
        .y_min = y_min,
        .y_max = y_max,
        .z_min = z_min,
        .z_max = z_max
    };
    device.GetQueue().WriteBuffer(boundaryCompute.paramsBuffer, 0, &params, sizeof(BoundaryParams));

    computePass.SetPipeline(boundaryCompute.pipeline);
    computePass.SetBindGroup(0, boundaryCompute.bindGroup);

    glm::u32 workgroupCount = (nParticles + 255) / 256;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
}
