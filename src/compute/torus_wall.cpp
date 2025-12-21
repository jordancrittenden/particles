#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/torus_wall.h"

struct TorusWallParams {
    glm::f32 r1;  // Major radius of torus
    glm::f32 r2;  // Minor radius of torus
};

TorusWallCompute create_torus_wall_compute(wgpu::Device& device, const ParticleBuffers& particleBuf, glm::u32 maxParticles) {
    TorusWallCompute torusWallCompute = {};

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/torus_wall.wgsl", {"kernel/physical_constants.wgsl"});
    if (!computeShaderModule) {
        std::cerr << "Failed to create torus wall compute shader module" << std::endl;
        exit(1);
    }

    // Create params uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Torus Wall Compute Params Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(TorusWallParams),
        .mappedAtCreation = false
    };
    torusWallCompute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    // Create compute bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        { // nParticles
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(glm::u32)
            }
        }, { // particlePos
            .binding = 1,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // particleVel
            .binding = 2,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // params
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(TorusWallParams)
            }
        }
    };

    wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc = {
        .label = "Torus Wall Compute Bind Group Layout",
        .entryCount = static_cast<uint32_t>(computeBindings.size()),
        .entries = computeBindings.data()
    };
    torusWallCompute.bindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    // Create compute pipeline
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Torus Wall Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &torusWallCompute.bindGroupLayout
    };
    wgpu::PipelineLayout computePipelineLayout = device.CreatePipelineLayout(&computePipelineLayoutDesc);

    wgpu::ComputePipelineDescriptor computePipelineDesc = {
        .label = "Torus Wall Compute Pipeline",
        .layout = computePipelineLayout,
        .compute = {
            .module = computeShaderModule,
            .entryPoint = "checkWallInteractions"
        }
    };
    torusWallCompute.pipeline = device.CreateComputePipeline(&computePipelineDesc);

    // Create compute bind group
    std::vector<wgpu::BindGroupEntry> computeEntries = {
        { // nParticles
            .binding = 0,
            .buffer = particleBuf.nCur,
            .offset = 0,
            .size = sizeof(uint32_t)
        }, { // particlePos
            .binding = 1,
            .buffer = particleBuf.pos,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // particleVel
            .binding = 2,
            .buffer = particleBuf.vel,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // params
            .binding = 3,
            .buffer = torusWallCompute.paramsBuffer,
            .offset = 0,
            .size = sizeof(TorusWallParams)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Torus Wall Compute Bind Group",
        .layout = torusWallCompute.bindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    torusWallCompute.bindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return torusWallCompute;
}

void run_torus_wall_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const TorusWallCompute& torusWallCompute,
    glm::f32 r1,
    glm::f32 r2,
    glm::u32 nParticles)
{
    // Update params buffer
    TorusWallParams params = {
        .r1 = r1,
        .r2 = r2
    };
    device.GetQueue().WriteBuffer(torusWallCompute.paramsBuffer, 0, &params, sizeof(TorusWallParams));

    // Set pipeline and bind group
    computePass.SetPipeline(torusWallCompute.pipeline);
    computePass.SetBindGroup(0, torusWallCompute.bindGroup);

    // Dispatch compute shader
    glm::u32 workgroupCount = (nParticles + 255) / 256;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
}
