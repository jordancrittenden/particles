#include <iostream>
#include <dawn/webgpu_cpp.h>
#include <glm/gtc/type_ptr.hpp>
#include "particles.h"
#include "util/wgpu_util.h"
#include "physical_constants.h"

ParticleRender create_particle_render(wgpu::Device& device) {
    ParticleRender render = {};
    
    // Create render shader module
    wgpu::ShaderModule renderShaderModule = create_shader_module(device, "shader/wgpu/particles.wgsl");
    if (!renderShaderModule) {
        std::cerr << "Failed to create render shader module" << std::endl;
        exit(1);
    }

    // Create uniform buffer for rendering
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Particle Uniform Buffer",
        .size = sizeof(glm::mat4) * 2,  // view, projection
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    render.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

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
    render.bindGroupLayout = device.CreateBindGroupLayout(&renderBindGroupLayoutDesc);

    // Create render pipeline layout
    wgpu::PipelineLayoutDescriptor renderPipelineLayoutDesc = {
        .label = "Render Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &render.bindGroupLayout
    };
    render.pipelineLayout = device.CreatePipelineLayout(&renderPipelineLayoutDesc);

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
        .writeMask = wgpu::ColorWriteMask::All
    };

    // Create fragment state
    wgpu::FragmentState fragmentState = {
        .module = renderShaderModule,
        .entryPoint = "fragmentMain",
        .targetCount = 1,
        .targets = &colorTarget
    };

    // Create depth stencil state
    wgpu::DepthStencilState depthStencilState = {
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .format = wgpu::TextureFormat::Depth24Plus
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor renderPipelineDesc = {
        .label = "Particle Render Pipeline",
        .layout = render.pipelineLayout,
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
    render.pipeline = device.CreateRenderPipeline(&renderPipelineDesc);

    // Create render bind group
    wgpu::BindGroupEntry renderBindGroupEntry = {
        .binding = 0,
        .buffer = render.uniformBuffer,
        .offset = 0,
        .size = sizeof(glm::mat4) * 2
    };

    wgpu::BindGroupDescriptor renderBindGroupDesc = {
        .layout = render.bindGroupLayout,
        .entryCount = 1,
        .entries = &renderBindGroupEntry
    };
    render.bindGroup = device.CreateBindGroup(&renderBindGroupDesc);

    return render;
}

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, const ParticleRender& render, glm::u32 nParticles, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    std::vector<glm::mat4> matrices = {view, projection};
    device.GetQueue().WriteBuffer(render.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 2);

    // Set the pipeline and bind group
    pass.SetPipeline(render.pipeline);
    pass.SetBindGroup(0, render.bindGroup);

    // Set the vertex buffer
    pass.SetVertexBuffer(0, particleBuf.pos, 0, nParticles * sizeof(glm::f32vec4));

    // Draw the particles
    pass.Draw(nParticles, 1, 0, 0);  // nParticles vertices, 1 instance, starting at vertex 0, instance 0
}
