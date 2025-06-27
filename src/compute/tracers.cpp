#include "compute/tracers.h"
#include "util/wgpu_util.h"
#include "shared/particles.h"
#include <iostream>

struct ETracerParams {
    glm::u32 nCurrentSegments;
    glm::f32 solenoidFlux;
    glm::u32 enableParticleFieldContributions;
    glm::u32 nTracers;
    glm::u32 tracerLength;
};

struct BTracerParams {
    glm::u32 nCurrentSegments;
    glm::u32 enableParticleFieldContributions;
    glm::u32 nTracers;
    glm::u32 tracerLength;
};

// Helper for E tracer subparts
void create_e_tracer_compute(
    wgpu::Device& device,
    TracerCompute& compute,
    const TracerBuffers& tracerBuf,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles)
{
    // Shader
    wgpu::ShaderModule eTracerShaderModule = create_shader_module(device, "kernel/e_tracer.wgsl", {"kernel/physical_constants.wgsl", "kernel/field_common.wgsl"});
    if (!eTracerShaderModule) {
        std::cerr << "Failed to create E tracer compute shader module" << std::endl;
        exit(1);
    }

    // Bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        { // nParticles
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(glm::u32)
            }
        }, { // traces (E or B)
            .binding = 1,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4)
            }
        }, { // pos
            .binding = 2,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // vel
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // currentSegments
            .binding = 4,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = nCurrentSegments * sizeof(glm::f32vec4) * 3
            }
        }, { // params (E or B)
            .binding = 5,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(ETracerParams)
            }
        }
    };
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .label = "E Tracer Bind Group Layout",
        .entryCount = computeBindings.size(),
        .entries = computeBindings.data()
    };
    compute.eBindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
        .label = "E Tracer Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &compute.eBindGroupLayout
    };
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Pipeline
    wgpu::ComputePipelineDescriptor ePipelineDesc = {
        .label = "E Tracer Compute Pipeline",
        .layout = pipelineLayout,
        .compute = {
            .module = eTracerShaderModule,
            .entryPoint = "updateTrails"
        }
    };
    compute.ePipeline = device.CreateComputePipeline(&ePipelineDesc);

    // Params buffer
    wgpu::BufferDescriptor eParamsBufferDesc = {
        .label = "E Tracer Params Buffer",
        .size = sizeof(ETracerParams),
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst
    };
    compute.eParamsBuffer = device.CreateBuffer(&eParamsBufferDesc);

    // Bind group
    std::vector<wgpu::BindGroupEntry> eBindGroupEntries = {
        { // nParticles
            .binding = 0,
            .buffer = particleBuf.nCur,
            .offset = 0,
            .size = sizeof(glm::u32)
        }, { // traces
            .binding = 1,
            .buffer = tracerBuf.e_traces,
            .offset = 0,
            .size = tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4)
        }, { // particlePos
            .binding = 2,
            .buffer = particleBuf.pos,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // particleVel
            .binding = 3,
            .buffer = particleBuf.vel,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // currentSegments
            .binding = 4,
            .buffer = currentSegmentsBuffer,
            .offset = 0,
            .size = nCurrentSegments * sizeof(glm::f32vec4) * 3
        }, { // params
            .binding = 5,
            .buffer = compute.eParamsBuffer,
            .offset = 0,
            .size = sizeof(ETracerParams)
        }
    };
    wgpu::BindGroupDescriptor eBindGroupDesc = {
        .layout = compute.eBindGroupLayout,
        .entryCount = eBindGroupEntries.size(),
        .entries = eBindGroupEntries.data()
    };
    compute.eBindGroup = device.CreateBindGroup(&eBindGroupDesc);
}

// Helper for B tracer subparts
void create_b_tracer_compute(
    wgpu::Device& device,
    TracerCompute& compute,
    const TracerBuffers& tracerBuf,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles)
{
    // Shader
    wgpu::ShaderModule bTracerShaderModule = create_shader_module(device, "kernel/b_tracer.wgsl", {"kernel/physical_constants.wgsl", "kernel/field_common.wgsl"});
    if (!bTracerShaderModule) {
        std::cerr << "Failed to create B tracer compute shader module" << std::endl;
        exit(1);
    }

    // Bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        { // nParticles
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(glm::u32)
            }
        }, { // traces (E or B)
            .binding = 1,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4)
            }
        }, { // pos
            .binding = 2,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // vel
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // currentSegments
            .binding = 4,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = nCurrentSegments * sizeof(glm::f32vec4) * 3
            }
        }, { // params (E or B)
            .binding = 5,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(BTracerParams)
            }
        }
    };
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .label = "B Tracer Bind Group Layout",
        .entryCount = computeBindings.size(),
        .entries = computeBindings.data()
    };
    compute.bBindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
        .label = "B Tracer Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &compute.bBindGroupLayout
    };
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Pipeline
    wgpu::ComputePipelineDescriptor bPipelineDesc = {
        .label = "B Tracer Compute Pipeline",
        .layout = pipelineLayout,
        .compute = {
            .module = bTracerShaderModule,
            .entryPoint = "updateTrails"
        }
    };
    compute.bPipeline = device.CreateComputePipeline(&bPipelineDesc);

    // Params buffer
    wgpu::BufferDescriptor bParamsBufferDesc = {
        .label = "B Tracer Params Buffer",
        .size = sizeof(BTracerParams),
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst
    };
    compute.bParamsBuffer = device.CreateBuffer(&bParamsBufferDesc);

    // Bind group
    std::vector<wgpu::BindGroupEntry> bBindGroupEntries = {
        { // nParticles
            .binding = 0,
            .buffer = particleBuf.nCur,
            .offset = 0,
            .size = sizeof(glm::u32)
        }, { // traces
            .binding = 1,
            .buffer = tracerBuf.b_traces,
            .offset = 0,
            .size = tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4)
        }, { // particlePos
            .binding = 2,
            .buffer = particleBuf.pos,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // particleVel
            .binding = 3,
            .buffer = particleBuf.vel,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // currentSegments
            .binding = 4,
            .buffer = currentSegmentsBuffer,
            .offset = 0,
            .size = nCurrentSegments * sizeof(glm::f32vec4) * 3
        }, { // params
            .binding = 5,
            .buffer = compute.bParamsBuffer,
            .offset = 0,
            .size = sizeof(BTracerParams)
        }
    };
    wgpu::BindGroupDescriptor bBindGroupDesc = {
        .layout = compute.bBindGroupLayout,
        .entryCount = bBindGroupEntries.size(),
        .entries = bBindGroupEntries.data()
    };
    compute.bBindGroup = device.CreateBindGroup(&bBindGroupDesc);
}

TracerCompute create_tracer_compute(
    wgpu::Device& device,
    const TracerBuffers& tracerBuf,
    const ParticleBuffers& particleBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles)
{
    TracerCompute compute = {};

    // E and B subparts
    create_e_tracer_compute(device, compute, tracerBuf, particleBuf, currentSegmentsBuffer, nCurrentSegments, maxParticles);
    create_b_tracer_compute(device, compute, tracerBuf, particleBuf, currentSegmentsBuffer, nCurrentSegments, maxParticles);

    return compute;
}

void run_tracer_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const TracerCompute& compute,
    glm::f32 dt,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions,
    glm::u32 nCurrentSegments,
    glm::u32 nParticles,
    glm::u32 nTracers,
    glm::u32 tracerLength) {
    
    // Update E tracer params buffer
    ETracerParams eParams = {
        .nCurrentSegments = nCurrentSegments,
        .solenoidFlux = solenoidFlux,
        .enableParticleFieldContributions = enableParticleFieldContributions,
        .nTracers = nTracers,
        .tracerLength = tracerLength
    };
    device.GetQueue().WriteBuffer(compute.eParamsBuffer, 0, &eParams, sizeof(ETracerParams));
    
    // Update B tracer params buffer
    BTracerParams bParams = {
        .nCurrentSegments = nCurrentSegments,
        .enableParticleFieldContributions = enableParticleFieldContributions,
        .nTracers = nTracers,
        .tracerLength = tracerLength
    };
    device.GetQueue().WriteBuffer(compute.bParamsBuffer, 0, &bParams, sizeof(BTracerParams));
    
    // Run E tracer compute
    computePass.SetPipeline(compute.ePipeline);
    computePass.SetBindGroup(0, compute.eBindGroup);
    computePass.DispatchWorkgroups(nTracers, 1, 1);
    
    // Run B tracer compute
    computePass.SetPipeline(compute.bPipeline);
    computePass.SetBindGroup(0, compute.bBindGroup);
    computePass.DispatchWorkgroups(nTracers, 1, 1);
}
