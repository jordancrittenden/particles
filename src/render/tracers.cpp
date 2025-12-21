#include <iostream>
#include "render/tracers.h"
#include "util/wgpu_util.h"
#include <glm/gtc/type_ptr.hpp>

struct Uniforms {
    glm::mat4 view;
    glm::mat4 projection;
    glm::f32vec4 color;
    glm::u32 headIdx;
    glm::u32 tracerLength;
    glm::f32 padding[2]; // Padding to align to 16-byte boundary
};

TracerRender create_tracer_render(wgpu::Device& device) {
    TracerRender render = {};
    
    // Create render shader module
    wgpu::ShaderModule renderShaderModule = create_shader_module(device, "shader/tracer.wgsl");
    if (!renderShaderModule) {
        std::cerr << "Failed to create tracer render shader module" << std::endl;
        exit(1);
    }
    
    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Tracer Uniform Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(Uniforms),
        .mappedAtCreation = false
    };
    render.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);
    
    // Create bind group layout
    wgpu::BindGroupLayoutEntry renderBinding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(Uniforms)
        }
    };
    
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .label = "Tracer Bind Group Layout",
        .entryCount = 1,
        .entries = &renderBinding
    };
    render.bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);
    
    // Create pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
        .label = "Tracer Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &render.bindGroupLayout
    };
    render.pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);
    
    // Create vertex state
    std::vector<wgpu::VertexAttribute> attributes = {
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        }
    };
    
    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(glm::f32vec4),
        .attributeCount = attributes.size(),
        .attributes = attributes.data()
    };
    
    // Create color target state
    wgpu::BlendState blendState = {
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha
        },
        .alpha = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha
        }
    };
    
    wgpu::ColorTargetState colorTarget = {
        .format = wgpu::TextureFormat::BGRA8Unorm,
        .blend = &blendState,
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
        .format = wgpu::TextureFormat::Depth24Plus,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Always
    };
    
    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .label = "Tracer Render Pipeline",
        .layout = render.pipelineLayout,
        .vertex = {
            .module = renderShaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::PointList
        },
        .depthStencil = &depthStencilState,
        .fragment = &fragmentState
    };
    render.pipeline = device.CreateRenderPipeline(&pipelineDesc);
    
    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {
        .binding = 0,
        .buffer = render.uniformBuffer,
        .offset = 0,
        .size = sizeof(Uniforms)
    };
    
    wgpu::BindGroupDescriptor bindGroupDesc = {
        .layout = render.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    render.bindGroup = device.CreateBindGroup(&bindGroupDesc);
    
    render.headIdx = 0;
    
    return render;
}

void render_e_tracers(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const TracerBuffers& tracerBuf,
    TracerRender& render,
    glm::mat4 view,
    glm::mat4 projection)
{
    // Update uniform buffer with view and projection matrices
    Uniforms uniforms = {
        .view = view,
        .projection = projection,
        .color = glm::f32vec4(1.0f, 0.0f, 0.0f, 1.0f),
        .headIdx = render.headIdx,
        .tracerLength = TRACER_LENGTH
    };
    
    device.GetQueue().WriteBuffer(render.uniformBuffer, 0, &uniforms, sizeof(Uniforms));
    
    // Set pipeline and bind group
    pass.SetPipeline(render.pipeline);
    pass.SetBindGroup(0, render.bindGroup);
    
    // Use the E tracer buffer directly as vertex buffer
    pass.SetVertexBuffer(0, tracerBuf.e_traces, 0, tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4));
    
    // Each tracer is TRACER_LENGTH points
    pass.Draw(TRACER_LENGTH * tracerBuf.nTracers, 1, 0, 0);

    render.headIdx = (render.headIdx + 1) % TRACER_LENGTH;
}

void render_b_tracers(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const TracerBuffers& tracerBuf,
    TracerRender& render,
    glm::mat4 view,
    glm::mat4 projection)
{
    // Update uniform buffer with view and projection matrices
    Uniforms uniforms = {
        .view = view,
        .projection = projection,
        .color = glm::f32vec4(0.0f, 0.0f, 1.0f, 1.0f),
        .headIdx = render.headIdx,
        .tracerLength = TRACER_LENGTH
    };
    
    device.GetQueue().WriteBuffer(render.uniformBuffer, 0, &uniforms, sizeof(Uniforms));
    
    // Set pipeline and bind group
    pass.SetPipeline(render.pipeline);
    pass.SetBindGroup(0, render.bindGroup);
    
    // Use the B tracer buffer directly as vertex buffer
    pass.SetVertexBuffer(0, tracerBuf.b_traces, 0, tracerBuf.nTracers * TRACER_LENGTH * sizeof(glm::f32vec4));
    
    // Each tracer is TRACER_LENGTH points
    pass.Draw(TRACER_LENGTH * tracerBuf.nTracers, 1, 0, 0);

    render.headIdx = (render.headIdx + 1) % TRACER_LENGTH;
}