#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/particles.h"

// C++ struct matching the WGSL ComputeMotionParams struct
struct ComputeMotionParams {
    glm::f32 dt;
    glm::u32 nCurrentSegments;
    glm::f32 solenoidFlux;
    glm::u32 enableParticleFieldContributions;
};

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

    // Read buffer for number of current particles
    wgpu::BufferDescriptor readBufDesc = {
        .label = "Particle Number Read Buffer",
        .size = sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .mappedAtCreation = false
    };
    compute.nCurReadBuf = device.CreateBuffer(&readBufDesc);

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
        .size = sizeof(ComputeMotionParams),
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
        }, { // currentSegments
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = nCurrentSegments * sizeof(glm::f32vec4) * 3
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
                .minBindingSize = sizeof(ComputeMotionParams)
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
        }, { // currentSegments
            .binding = 3,
            .buffer = currentSegmentsBuffer,
            .offset = 0,
            .size = nCurrentSegments * sizeof(glm::f32vec4) * 3
        }, { // debug
            .binding = 4,
            .buffer = compute.debugBuffer,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // params
            .binding = 5,
            .buffer = compute.paramsBuffer,
            .offset = 0,
            .size = sizeof(ComputeMotionParams)
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
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments)
{
    // Update params buffer
    ComputeMotionParams params = {
        .dt = dt,
        .nCurrentSegments = nCurrentSegments,
        .solenoidFlux = solenoidFlux,
        .enableParticleFieldContributions = enableParticleFieldContributions
    };
    device.GetQueue().WriteBuffer(compute.paramsBuffer, 0, &params, sizeof(ComputeMotionParams));

    computePass.SetPipeline(compute.pipeline);
    computePass.SetBindGroup(0, compute.bindGroup);
    computePass.DispatchWorkgroups(256, 1, 1);
}

glm::u32 read_nparticles(wgpu::Device& device, wgpu::Instance& instance, const ParticleCompute& compute) {
    const void* nParticlesRaw = read_buffer(device, instance, compute.nCurReadBuf, sizeof(glm::u32));
    return *reinterpret_cast<const glm::u32*>(nParticlesRaw);
}