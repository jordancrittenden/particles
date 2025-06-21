#include <iostream>
#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include "solenoid.h"
#include "ring.h"

struct UniformData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::f32 solenoidFlux;
    glm::f32 padding[3]; // Padding to align to 16-byte boundary
};

SolenoidBuffers create_solenoid_buffers(wgpu::Device& device, const Ring& ring) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/wgpu/solenoid.wgsl");
    if (!shaderModule) {
        std::cerr << "Failed to create solenoid shader module" << std::endl;
        exit(1);
    }

    SolenoidBuffers buf = {};

    // Generate solenoid vertices and indices using the ring vertex generation function
    std::vector<glm::f32> vertices;
    generate_ring_vertices(ring, vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Solenoid Vertex Buffer",
        .size = vertices.size() * sizeof(glm::f32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Solenoid Index Buffer",
        .size = buf.indices.size() * sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .mappedAtCreation = false
    };
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Solenoid Uniform Buffer",
        .size = sizeof(UniformData),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    buf.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create bind group layout
    wgpu::BindGroupLayoutEntry binding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(UniformData)
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
        // Normal
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
        .label = "Solenoid Render Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .fragment = &fragmentState,
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .frontFace = wgpu::FrontFace::CCW
        },
        .depthStencil = &depthStencilState
    };
    buf.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {
        .binding = 0,
        .buffer = buf.uniformBuffer,
        .offset = 0,
        .size = sizeof(UniformData)
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .label = "Solenoid Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_solenoid(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const SolenoidBuffers& solenoidBuf, glm::f32 solenoidFlux, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices and solenoidFlux
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    UniformData uniformData = {
        .model = model,
        .view = view,
        .projection = projection,
        .solenoidFlux = solenoidFlux,
        .padding = {0.0f, 0.0f, 0.0f}
    };
    
    device.GetQueue().WriteBuffer(solenoidBuf.uniformBuffer, 0, &uniformData, sizeof(UniformData));

    // Set the pipeline and bind group
    pass.SetPipeline(solenoidBuf.pipeline);
    pass.SetBindGroup(0, solenoidBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, solenoidBuf.vertexBuffer, 0, solenoidBuf.vertexBuffer.GetSize());
    pass.SetIndexBuffer(solenoidBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, solenoidBuf.indexBuffer.GetSize());

    // Draw the solenoid
    pass.DrawIndexed(solenoidBuf.indices.size(), 1, 0, 0, 0);
} 