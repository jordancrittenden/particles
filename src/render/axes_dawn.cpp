#include <iostream>
#include <dawn/webgpu_cpp.h>
#include <glm/gtc/type_ptr.hpp>
#include "axes_dawn.h"
#include "../util/wgpu_util.h"
#include "physical_constants.h"

AxesBuffers create_axes_buffers(wgpu::Device& device) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/wgpu/axes.wgsl");
    if (!shaderModule) {
        std::cerr << "Failed to create axes shader module" << std::endl;
        exit(1);
    }

    AxesBuffers buf = {};

    // Create vertex buffer
    float axisVertices[] = {
        // X-axis
        -10.0f * _M, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Red for X
         10.0f * _M, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        // Y-axis
        0.0f, -10.0f * _M, 0.0f,   0.0f, 1.0f, 0.0f,  // Green for Y
        0.0f,  10.0f * _M, 0.0f,   0.0f, 1.0f, 0.0f,
        // Z-axis
        0.0f, 0.0f, -10.0f * _M,   0.0f, 0.0f, 1.0f,  // Blue for Z
        0.0f, 0.0f,  10.0f * _M,   0.0f, 0.0f, 1.0f
    };

    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Axes Vertex Buffer",
        .size = sizeof(axisVertices),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, axisVertices, sizeof(axisVertices));
    
    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Axes Uniform Buffer",
        .size = sizeof(glm::mat4) * 3,  // model, view, projection
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    buf.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create bind group layout
    wgpu::BindGroupLayoutEntry binding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(glm::mat4) * 3
        }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .entryCount = 1,
        .entries = &binding
    };
    buf.bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Create pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &buf.bindGroupLayout
    };
    buf.pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create vertex state
    std::vector<wgpu::VertexAttribute> attributes = {
        // Position
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        },
        // Color
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 3 * sizeof(glm::f32),
            .shaderLocation = 1
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = 6 * sizeof(glm::f32),
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
        .module = shaderModule,
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
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .label = "Axes Render Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .fragment = &fragmentState,
        .primitive = {
            .topology = wgpu::PrimitiveTopology::LineList
        },
        .depthStencil = &depthStencilState
    };
    buf.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {
        .binding = 0,
        .buffer = buf.uniformBuffer,
        .offset = 0,
        .size = sizeof(glm::mat4) * 3
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .label = "Axes Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_axes(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const AxesBuffers& axesBuf, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    glm::mat4 model = glm::mat4(1.0f);
    std::vector<glm::mat4> matrices = {model, view, projection};
    device.GetQueue().WriteBuffer(axesBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 3);

    // Set the pipeline and bind group
    pass.SetPipeline(axesBuf.pipeline);
    pass.SetBindGroup(0, axesBuf.bindGroup);

    // Set the vertex buffer
    pass.SetVertexBuffer(0, axesBuf.vertexBuffer, 0, 6 * sizeof(float) * 6);  // 6 vertices, 6 floats each

    // Draw the axes
    pass.Draw(6, 1, 0, 0);  // 6 vertices, 1 instance, starting at vertex 0, instance 0
}
