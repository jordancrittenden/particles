#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/particles.h"

ParticleCompute create_particle_compute(
    wgpu::Device& device,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles)
{
    ParticleCompute compute = {};

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/wgpu/particles.wgsl", {"kernel/wgpu/physical_constants.wgsl", "kernel/wgpu/field_common.wgsl"});
    if (!computeShaderModule) {
        std::cerr << "Failed to create compute shader module" << std::endl;
        exit(1);
    }

    // Create persistent compute buffers
    wgpu::BufferDescriptor nParticlesBufferDesc = {
        .label = "nParticles Buffer",
        .size = sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    compute.nParticlesBuffer = device.CreateBuffer(&nParticlesBufferDesc);

    // Create debug buffer
    wgpu::BufferDescriptor debugBufferDesc = {
        .label = "Debug Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    compute.debugBuffer = device.CreateBuffer(&debugBufferDesc);

    // Create params uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Compute Params Buffer",
        .size = sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    compute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    // Create compute bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        { // nParticles
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(uint32_t)
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
        }, { // currentSegments
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = 1024 * sizeof(glm::f32vec4)
            }
        }, { // debug
            .binding = 4,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // params
            .binding = 5,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(glm::f32vec4)
            }
        }
    };

    wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc = {
        .label = "Compute Bind Group Layout",
        .entryCount = static_cast<uint32_t>(computeBindings.size()),
        .entries = computeBindings.data()
    };
    compute.bindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    // Create compute pipeline
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &compute.bindGroupLayout
    };
    wgpu::PipelineLayout computePipelineLayout = device.CreatePipelineLayout(&computePipelineLayoutDesc);

    wgpu::ComputePipelineDescriptor computePipelineDesc = {
        .label = "Particle Compute Pipeline",
        .layout = computePipelineLayout,
        .compute = {
            .module = computeShaderModule,
            .entryPoint = "computeMotion"
        }
    };
    compute.pipeline = device.CreateComputePipeline(&computePipelineDesc);

    // Create compute bind group with persistent buffers
    std::vector<wgpu::BindGroupEntry> computeEntries = {
        { // nParticles
            .binding = 0,
            .buffer = compute.nParticlesBuffer,
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
        }, { // currentSegments
            .binding = 3,
            .buffer = currentSegmentsBuffer,
            .offset = 0,
            .size = 1024 * sizeof(glm::f32vec4)
        }, { // debug
            .binding = 4,
            .buffer = compute.debugBuffer,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // params
            .binding = 5,
            .buffer = compute.paramsBuffer,
            .offset = 0,
            .size = sizeof(glm::f32vec4)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Compute Bind Group",
        .layout = compute.bindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    compute.bindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return compute;
}

void run_particle_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& compute,
    glm::u32 nParticles,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments)
{
    // Update persistent buffers with current values
    device.GetQueue().WriteBuffer(compute.nParticlesBuffer, 0, &nParticles, sizeof(uint32_t));

    // Update params buffer
    glm::f32vec4 params = {
        dt,
        static_cast<float>(nCurrentSegments),
        solenoidFlux,
        static_cast<float>(enableParticleFieldContributions)
    };
    device.GetQueue().WriteBuffer(compute.paramsBuffer, 0, &params, sizeof(glm::f32vec4));

    computePass.SetPipeline(compute.pipeline);
    computePass.SetBindGroup(0, compute.bindGroup);
    
    // Calculate workgroup count (256 threads per workgroup)
    uint32_t workgroupCount = (nParticles + 255) / 256;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
}