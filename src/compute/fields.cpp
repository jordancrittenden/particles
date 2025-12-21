#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/fields.h"
#include "mesh.h"

// C++ struct matching the WGSL ComputeFieldsParams struct
struct ComputeFieldsParams {
    glm::u32 nCells;
    glm::u32 nCurrentSegments;
    glm::f32 solenoidFlux;
    glm::u32 enableParticleFieldContributions; 
};

FieldCompute create_field_compute(
    wgpu::Device& device,
    const std::vector<Cell>& cells,
    const ParticleBuffers& particleBuf,
    const FieldBuffers& fieldBuf,
    const wgpu::Buffer& currentSegmentsBuffer,
    glm::u32 nCurrentSegments,
    glm::u32 maxParticles
) {
    FieldCompute fieldCompute = {};

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/fields.wgsl", {"kernel/physical_constants.wgsl", "kernel/field_common.wgsl"});
    if (!computeShaderModule) {
        std::cerr << "Failed to create compute shader module" << std::endl;
        exit(1);
    }

    // Create cell location buffer
    std::vector<glm::f32vec4> cellLocations;
    for (const auto& cell : cells) {
        cellLocations.push_back(cell.pos);
    }
    wgpu::BufferDescriptor cellLocationBufferDesc = {
        .label = "Cell Location Buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = cellLocations.size() * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    fieldCompute.cellLocationBuffer = device.CreateBuffer(&cellLocationBufferDesc);
    device.GetQueue().WriteBuffer(fieldCompute.cellLocationBuffer, 0, cellLocations.data(), cellLocations.size() * sizeof(glm::f32vec4));
    
    // Create debug buffer
    wgpu::BufferDescriptor debugBufferDesc = {
        .label = "Debug Buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = cells.size() * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    fieldCompute.debugBuffer = device.CreateBuffer(&debugBufferDesc);

    // Create params uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Fields Params Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(ComputeFieldsParams),
        .mappedAtCreation = false
    };
    fieldCompute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    // Create compute bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> computeBindings = {
        { // nParticles
            .binding = 0,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = sizeof(glm::u32)
            }
        }, { // cellLocation
            .binding = 1,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = cellLocations.size() * sizeof(glm::f32vec4)
            }
        }, { // eField
            .binding = 2,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = cells.size() * sizeof(glm::f32vec4)
            }
        }, { // bField
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = cells.size() * sizeof(glm::f32vec4)
            }
        }, { // particlePos
            .binding = 4,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // particleVel
            .binding = 5,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // currentSegments
            .binding = 6,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = nCurrentSegments * sizeof(glm::f32vec4) * 3
            }
        }, { // debug
            .binding = 7,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = cells.size() * sizeof(glm::f32vec4)
            }
        }, { // params
            .binding = 8,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(ComputeFieldsParams)
            }
        }
    };
    
    wgpu::BindGroupLayoutDescriptor computeBindGroupLayoutDesc = {
        .label = "Field Compute Bind Group Layout",
        .entryCount = computeBindings.size(),
        .entries = computeBindings.data()
    };
    fieldCompute.bindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);
    
    // Create compute pipeline
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Particle Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &fieldCompute.bindGroupLayout
    };
    wgpu::PipelineLayout computePipelineLayout = device.CreatePipelineLayout(&computePipelineLayoutDesc);

    wgpu::ComputePipelineDescriptor computePipelineDesc = {
        .label = "Field Compute Pipeline",
        .layout = computePipelineLayout,
        .compute = {
            .module = computeShaderModule,
            .entryPoint = "computeFields"
        }
    };
    fieldCompute.pipeline = device.CreateComputePipeline(&computePipelineDesc);

    // Create compute bind group with persistent buffers
    std::vector<wgpu::BindGroupEntry> computeEntries = {
        { // nParticles
            .binding = 0,
            .buffer = particleBuf.nCur,
            .offset = 0,
            .size = sizeof(glm::u32)
        }, { // cellLocation
            .binding = 1,
            .buffer = fieldCompute.cellLocationBuffer,
            .offset = 0,
            .size = cells.size() * sizeof(glm::f32vec4)
        }, { // eField
            .binding = 2,
            .buffer = fieldBuf.eField,
            .offset = 0,
            .size = cells.size() * sizeof(glm::f32vec4)
        }, { // bField
            .binding = 3,
            .buffer = fieldBuf.bField,
            .offset = 0,
            .size = cells.size() * sizeof(glm::f32vec4)
        }, { // particlePos
            .binding = 4,
            .buffer = particleBuf.pos,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // particleVel
            .binding = 5,
            .buffer = particleBuf.vel,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // currentSegments
            .binding = 6,
            .buffer = currentSegmentsBuffer,
            .offset = 0,
            .size = nCurrentSegments * sizeof(glm::f32vec4) * 3
        }, { // debug
            .binding = 7,
            .buffer = fieldCompute.debugBuffer,
            .offset = 0,
            .size = cells.size() * sizeof(glm::f32vec4)
        }, { // params
            .binding = 8,
            .buffer = fieldCompute.paramsBuffer,
            .offset = 0,
            .size = sizeof(ComputeFieldsParams)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Field Compute Bind Group",
        .layout = fieldCompute.bindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    fieldCompute.bindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return fieldCompute;
}

void run_field_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& pass,
    FieldCompute& fieldCompute,
    glm::u32 nCells,
    glm::u32 nCurrentSegments,
    glm::f32 solenoidFlux,
    glm::u32 enableParticleFieldContributions)
{
    // Update params buffer
    ComputeFieldsParams params = {
        .nCells = nCells,
        .nCurrentSegments = nCurrentSegments,
        .solenoidFlux = solenoidFlux,
        .enableParticleFieldContributions = enableParticleFieldContributions
    };
    device.GetQueue().WriteBuffer(fieldCompute.paramsBuffer, 0, &params, sizeof(ComputeFieldsParams));

    glm::u32 nWorkgroups = (nCells + 255) / 256;

    pass.SetPipeline(fieldCompute.pipeline);
    pass.SetBindGroup(0, fieldCompute.bindGroup);
    pass.DispatchWorkgroups(nWorkgroups, 1, 1);
} 