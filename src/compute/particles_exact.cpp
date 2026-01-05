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
    ParticleCompute particleCompute = {};

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/particles_exact.wgsl", {"kernel/physical_constants.wgsl", "kernel/field_common.wgsl"});
    if (!computeShaderModule) {
        std::cerr << "Failed to create compute shader module" << std::endl;
        exit(1);
    }

    // Read buffer for number of current particles
    wgpu::BufferDescriptor nParticlesReadBufDesc = {
        .label = "Particle Number Read Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = sizeof(glm::u32),
        .mappedAtCreation = false
    };
    particleCompute.nParticlesReadBuf = device.CreateBuffer(&nParticlesReadBufDesc);

    // Create debug storage buffer for compute shader
    wgpu::BufferDescriptor debugStorageBufDesc = {
        .label = "Particle Debug Storage Buffer",
        .usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage,
        .size = maxParticles * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    particleCompute.debugStorageBuf = device.CreateBuffer(&debugStorageBufDesc);

    // Create debug read buffer for CPU access
    wgpu::BufferDescriptor debugReadBufDesc = {
        .label = "Particle Debug Read Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = maxParticles * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    particleCompute.debugReadBuf = device.CreateBuffer(&debugReadBufDesc);

    // Create params uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Particle Compute Params Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(ComputeMotionParams),
        .mappedAtCreation = false
    };
    particleCompute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

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
        .label = "Particle Compute Bind Group Layout",
        .entryCount = static_cast<uint32_t>(computeBindings.size()),
        .entries = computeBindings.data()
    };
    particleCompute.bindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    // Create compute pipeline
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Particle Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &particleCompute.bindGroupLayout
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
    particleCompute.pipeline = device.CreateComputePipeline(&computePipelineDesc);

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
            .buffer = particleCompute.debugStorageBuf,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // params
            .binding = 5,
            .buffer = particleCompute.paramsBuffer,
            .offset = 0,
            .size = sizeof(ComputeMotionParams)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Particle Compute Bind Group",
        .layout = particleCompute.bindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    particleCompute.bindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return particleCompute;
}

void run_particle_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& particleCompute,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments,
    glm::u32 nParticles)
{
    // Update params buffer
    ComputeMotionParams params = {
        .dt = dt,
        .nCurrentSegments = nCurrentSegments,
        .solenoidFlux = solenoidFlux,
        .enableParticleFieldContributions = enableParticleFieldContributions
    };
    device.GetQueue().WriteBuffer(particleCompute.paramsBuffer, 0, &params, sizeof(ComputeMotionParams));

    // Each workgroup processes 16 particles (workgroup_size(16))
    glm::u32 workgroupSize = 16;
    glm::u32 nWorkgroups = (nParticles + workgroupSize - 1) / workgroupSize;

    computePass.SetPipeline(particleCompute.pipeline);
    computePass.SetBindGroup(0, particleCompute.bindGroup);
    computePass.DispatchWorkgroups(nWorkgroups, 1, 1);
}

glm::u32 read_nparticles(wgpu::Device& device, wgpu::Instance& instance, const ParticleCompute& compute) {
    const void* nParticlesRaw = read_buffer(device, instance, compute.nParticlesReadBuf, sizeof(glm::u32));
    return *reinterpret_cast<const glm::u32*>(nParticlesRaw);
}

void read_particles_debug(wgpu::Device& device, wgpu::Instance& instance, const ParticleCompute& compute, std::vector<glm::f32vec4>& debug, glm::u32 n) {
    const void* debugRaw = read_buffer(device, instance, compute.debugReadBuf, n * sizeof(glm::f32vec4));
    const glm::f32vec4* debugStart = reinterpret_cast<const glm::f32vec4*>(debugRaw);
    const glm::f32vec4* debugEnd = debugStart + n;
    debug.assign(debugStart, debugEnd);
}