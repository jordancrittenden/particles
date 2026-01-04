#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "util/wgpu_util.h"
#include "compute/particles.h"
#include "mesh.h"

struct MeshPropertiesUniform {
    glm::f32vec3 min; // minimum cell center
    glm::f32 _padding1; // padding for 16-byte alignment
    glm::f32vec3 max; // maximum cell center
    glm::f32 _padding2; // padding for 16-byte alignment
    glm::u32vec3 dim; // number of cells in each dimension
    glm::u32 _padding3; // padding for 16-byte alignment
    glm::f32vec3 cell_size; // cell size
    glm::f32 _padding4; // padding for 16-byte alignment
};

ParticleCompute create_particle_pic_compute(
    wgpu::Device& device,
    const std::vector<Cell>& cells,
    const ParticleBuffers& particleBuf,
    const FieldBuffers& fieldBuf,
    glm::u32 maxParticles)
{
    ParticleCompute particleCompute = {};

    // Create cell location buffer
    glm::u32 nCells = static_cast<glm::u32>(cells.size());
    std::vector<glm::f32vec4> cellLocations;
    for (const auto& cell : cells) {
        cellLocations.push_back(cell.pos);
    }
    wgpu::BufferDescriptor cellLocationBufferDesc = {
        .label = "Cell Location Buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        .size = nCells * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    particleCompute.cellLocationBuffer = device.CreateBuffer(&cellLocationBufferDesc);
    device.GetQueue().WriteBuffer(particleCompute.cellLocationBuffer, 0, cellLocations.data(), nCells * sizeof(glm::f32vec4));

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(
        device,
        "kernel/particles_pic.wgsl",
        {
            "kernel/physical_constants.wgsl",
            "kernel/field_common.wgsl",
            "kernel/mesh.wgsl"
        }
    );
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

    // Create dt uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Particle Compute Params Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(glm::f32),
        .mappedAtCreation = false
    };
    particleCompute.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    // Create mesh uniform buffer
    wgpu::BufferDescriptor meshBufferDesc = {
        .label = "Particle Compute Mesh Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(MeshPropertiesUniform),
        .mappedAtCreation = false
    };
    particleCompute.meshBuffer = device.CreateBuffer(&meshBufferDesc);

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
        }, { // eField
            .binding = 3,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = fieldBuf.nCells * sizeof(glm::f32vec4)
            }
        }, { // bField
            .binding = 4,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = fieldBuf.nCells * sizeof(glm::f32vec4)
            }
        }, { // debug
            .binding = 5,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Storage,
                .minBindingSize = maxParticles * sizeof(glm::f32vec4)
            }
        }, { // dt
            .binding = 6,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(glm::f32)
            }
        }, { // mesh
            .binding = 7,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(MeshPropertiesUniform)
            }
        }, { // cellLocation
            .binding = 8,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = nCells * sizeof(glm::f32vec4)
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
        }, { // eField
            .binding = 3,
            .buffer = fieldBuf.eField,
            .offset = 0,
            .size = fieldBuf.nCells * sizeof(glm::f32vec4)
        }, { // bField
            .binding = 4,
            .buffer = fieldBuf.bField,
            .offset = 0,
            .size = fieldBuf.nCells * sizeof(glm::f32vec4)
        }, { // debug
            .binding = 5,
            .buffer = particleCompute.debugStorageBuf,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // dt
            .binding = 6,
            .buffer = particleCompute.paramsBuffer,
            .offset = 0,
            .size = sizeof(glm::f32)
        }, { // mesh
            .binding = 7,
            .buffer = particleCompute.meshBuffer,
            .offset = 0,
            .size = sizeof(MeshPropertiesUniform)
        }, { // cellLocation
            .binding = 8,
            .buffer = particleCompute.cellLocationBuffer,
            .offset = 0,
            .size = nCells * sizeof(glm::f32vec4)
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

void run_particle_pic_compute(
    wgpu::Device& device,
    wgpu::ComputePassEncoder& computePass,
    const ParticleCompute& particleCompute,
    const MeshProperties& mesh,
    glm::f32 dt,
    glm::u32 nParticles)
{
    // Update dt
    device.GetQueue().WriteBuffer(particleCompute.paramsBuffer, 0, &dt, sizeof(glm::f32));

    MeshPropertiesUniform meshUniform = {
        .min = mesh.min,
        .max = mesh.max,
        .dim = mesh.dim,
        .cell_size = mesh.cell_size
    };
    device.GetQueue().WriteBuffer(particleCompute.meshBuffer, 0, &meshUniform, sizeof(MeshPropertiesUniform));

    // Each workgroup processes 16 particles (workgroup_size(16))
    glm::u32 workgroupSize = 16;
    glm::u32 nWorkgroups = (nParticles + workgroupSize - 1) / workgroupSize;

    computePass.SetPipeline(particleCompute.pipeline);
    computePass.SetBindGroup(0, particleCompute.bindGroup);
    computePass.DispatchWorkgroups(nWorkgroups, 1, 1);
}