#include <iostream>
#include <dawn/webgpu_cpp.h>
#include <glm/gtc/type_ptr.hpp>
#include "particles_dawn.h"
#include "../util/wgpu_util.h"
#include "physical_constants.h"

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::f32vec4()> posF,
    std::function<glm::f32vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    glm::u32 initialParticles,
    glm::u32 maxParticles) {

    ParticleBuffers buf = {};

    // Create compute shader module
    wgpu::ShaderModule computeShaderModule = create_shader_module(device, "kernel/wgpu/particles.wgsl", {"kernel/wgpu/physical_constants.wgsl", "kernel/wgpu/field_common.wgsl"});
    if (!computeShaderModule) {
        std::cerr << "Failed to create compute shader module" << std::endl;
        exit(1);
    }
    
    // Create render shader module
    wgpu::ShaderModule renderShaderModule = create_shader_module(device, "shader/wgpu/particles.wgsl");
    if (!renderShaderModule) {
        std::cerr << "Failed to create render shader module" << std::endl;
        exit(1);
    }

    std::vector<glm::f32vec4> position_and_type;
    std::vector<glm::f32vec4> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < maxParticles; ++i) {
        if (i < initialParticles) {
            PARTICLE_SPECIES species = speciesF();
            glm::f32vec4 pos = posF();
            glm::f32vec4 vel = velF(species);

            pos[3] = (float)species;

            // [x, y, z, species]
            position_and_type.push_back(pos);
            // [dx, dy, dz, unused]
            velocity.push_back(vel);
        } else {
            // Placeholders for future particles that may be created via collisions

            // [x, y, z, species]
            position_and_type.push_back(glm::f32vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
            // [dx, dy, dz, unused]
            velocity.push_back(glm::f32vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
        }
    }

    // Create shared position buffer (used by both compute and render)
    wgpu::BufferDescriptor sharedPosBufferDesc = {
        .label = "Shared Particle Position Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.sharedPosBuffer = device.CreateBuffer(&sharedPosBufferDesc);
    device.GetQueue().WriteBuffer(buf.sharedPosBuffer, 0, position_and_type.data(), position_and_type.size() * sizeof(glm::f32vec4));

    // Create shared velocity buffer (used by both compute and render)
    wgpu::BufferDescriptor sharedVelBufferDesc = {
        .label = "Shared Particle Velocity Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.sharedVelBuffer = device.CreateBuffer(&sharedVelBufferDesc);
    device.GetQueue().WriteBuffer(buf.sharedVelBuffer, 0, velocity.data(), velocity.size() * sizeof(glm::f32vec4));

    // Create persistent compute buffers
    wgpu::BufferDescriptor nParticlesBufferDesc = {
        .label = "nParticles Buffer",
        .size = sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.nParticlesBuffer = device.CreateBuffer(&nParticlesBufferDesc);

    // Create placeholder currentSegments buffer (will be updated with actual data later)
    std::vector<glm::f32vec4> placeholderCurrentSegments(1024, glm::f32vec4(0.0f));
    wgpu::BufferDescriptor currentSegmentsBufferDesc = {
        .label = "Current Segments Buffer",
        .size = placeholderCurrentSegments.size() * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.currentSegmentsBuffer = device.CreateBuffer(&currentSegmentsBufferDesc);
    device.GetQueue().WriteBuffer(buf.currentSegmentsBuffer, 0, placeholderCurrentSegments.data(), placeholderCurrentSegments.size() * sizeof(glm::f32vec4));

    // Create debug buffer
    wgpu::BufferDescriptor debugBufferDesc = {
        .label = "Debug Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.debugBuffer = device.CreateBuffer(&debugBufferDesc);

    // Create params uniform buffer
    wgpu::BufferDescriptor paramsBufferDesc = {
        .label = "Compute Params Buffer",
        .size = sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    buf.paramsBuffer = device.CreateBuffer(&paramsBufferDesc);

    // Create uniform buffer for rendering
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Particle Uniform Buffer",
        .size = sizeof(glm::mat4) * 2,  // view, projection
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    buf.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create render bind group layout
    wgpu::BindGroupLayoutEntry renderBinding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(glm::mat4) * 2
        }
    };

    wgpu::BindGroupLayoutDescriptor renderBindGroupLayoutDesc = {
        .label = "Render Bind Group Layout",
        .entryCount = 1,
        .entries = &renderBinding
    };
    buf.bindGroupLayout = device.CreateBindGroupLayout(&renderBindGroupLayoutDesc);

    // Create render pipeline layout
    wgpu::PipelineLayoutDescriptor renderPipelineLayoutDesc = {
        .label = "Render Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &buf.bindGroupLayout
    };
    buf.pipelineLayout = device.CreatePipelineLayout(&renderPipelineLayoutDesc);

    // Create vertex state for rendering
    std::vector<wgpu::VertexAttribute> attributes = {
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        }, {
            .format = wgpu::VertexFormat::Float32,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(glm::f32vec4),
        .attributeCount = attributes.size(),
        .attributes = attributes.data()
    };

    // Create color state
    wgpu::ColorTargetState colorTarget = {
        .format = wgpu::TextureFormat::BGRA8Unorm,
        .blend = nullptr,
        .writeMask = wgpu::ColorWriteMask::All
    };

    // Create fragment state
    wgpu::FragmentState fragmentState = {
        .module = renderShaderModule,
        .entryPoint = "fragmentMain",
        .targetCount = 1,
        .targets = &colorTarget
    };

    wgpu::DepthStencilState depthStencilState = {
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .format = wgpu::TextureFormat::Depth24Plus
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor renderPipelineDesc = {
        .label = "Particle Render Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = renderShaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .fragment = &fragmentState,
        .primitive = {
            .topology = wgpu::PrimitiveTopology::PointList
        },
        .depthStencil = &depthStencilState
    };
    buf.pipeline = device.CreateRenderPipeline(&renderPipelineDesc);

    // Create render bind group
    wgpu::BindGroupEntry renderBindGroupEntry = {
        .binding = 0,
        .buffer = buf.uniformBuffer,
        .offset = 0,
        .size = sizeof(glm::mat4) * 2
    };

    wgpu::BindGroupDescriptor renderBindGroupDesc = {
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &renderBindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&renderBindGroupDesc);

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
    buf.computeBindGroupLayout = device.CreateBindGroupLayout(&computeBindGroupLayoutDesc);

    // Create compute pipeline
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDesc = {
        .label = "Compute Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &buf.computeBindGroupLayout
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
    buf.computePipeline = device.CreateComputePipeline(&computePipelineDesc);

    // Create compute bind group with persistent buffers
    std::vector<wgpu::BindGroupEntry> computeEntries = {
        { // nParticles
            .binding = 0,
            .buffer = buf.nParticlesBuffer,
            .offset = 0,
            .size = sizeof(uint32_t)
        }, { // particlePos
            .binding = 1,
            .buffer = buf.sharedPosBuffer,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // particleVel
            .binding = 2,
            .buffer = buf.sharedVelBuffer,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // currentSegments
            .binding = 3,
            .buffer = buf.currentSegmentsBuffer,
            .offset = 0,
            .size = placeholderCurrentSegments.size() * sizeof(glm::f32vec4)
        }, { // debug
            .binding = 4,
            .buffer = buf.debugBuffer,
            .offset = 0,
            .size = maxParticles * sizeof(glm::f32vec4)
        }, { // params
            .binding = 5,
            .buffer = buf.paramsBuffer,
            .offset = 0,
            .size = sizeof(glm::f32vec4)
        }
    };

    wgpu::BindGroupDescriptor computeBindGroupDesc = {
        .label = "Compute Bind Group",
        .layout = buf.computeBindGroupLayout,
        .entryCount = static_cast<uint32_t>(computeEntries.size()),
        .entries = computeEntries.data()
    };
    buf.computeBindGroup = device.CreateBindGroup(&computeBindGroupDesc);

    return buf;
}

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, int nParticles, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    std::vector<glm::mat4> matrices = {view, projection};
    device.GetQueue().WriteBuffer(particleBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 2);

    // Set the pipeline and bind group
    pass.SetPipeline(particleBuf.pipeline);
    pass.SetBindGroup(0, particleBuf.bindGroup);

    // Set the vertex buffer
    pass.SetVertexBuffer(0, particleBuf.sharedPosBuffer, 0, nParticles * sizeof(glm::f32vec4));

    // Draw the particles
    pass.Draw(nParticles, 1, 0, 0);  // nParticles vertices, 1 instance, starting at vertex 0, instance 0
}

void run_particle_compute(wgpu::Device& device, wgpu::ComputePassEncoder& computePass, const ParticleBuffers& particleBuf, glm::u32 nParticles, glm::f32 dt, float solenoidFlux, glm::u32 enableParticleFieldContributions) {
    // Update persistent buffers with current values
    device.GetQueue().WriteBuffer(particleBuf.nParticlesBuffer, 0, &nParticles, sizeof(uint32_t));

    // Update params buffer
    glm::f32vec4 params = {
        dt,
        0.0f,
        solenoidFlux,
        static_cast<float>(enableParticleFieldContributions)
    };
    device.GetQueue().WriteBuffer(particleBuf.paramsBuffer, 0, &params, sizeof(glm::f32vec4));

    computePass.SetPipeline(particleBuf.computePipeline);
    computePass.SetBindGroup(0, particleBuf.computeBindGroup);
    
    // Calculate workgroup count (256 threads per workgroup)
    uint32_t workgroupCount = (nParticles + 255) / 256;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
}
