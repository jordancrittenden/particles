#include <iostream>
#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include "torus.h"

struct UniformData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::f32 padding[4]; // Padding to align to 16-byte boundary
};

// Generate torus vertices and indices
void generate_torus_vertices(float r1, float r2, int toroidalSegments, int poloidalSegments, 
                           std::vector<glm::f32>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();
    
    // Generate vertices
    for (int i = 0; i <= toroidalSegments; i++) {
        float toroidalAngle = 2.0f * M_PI * i / toroidalSegments;
        float cosToroidal = cos(toroidalAngle);
        float sinToroidal = sin(toroidalAngle);
        
        for (int j = 0; j <= poloidalSegments; j++) {
            float poloidalAngle = 2.0f * M_PI * j / poloidalSegments;
            float cosPoloidal = cos(poloidalAngle);
            float sinPoloidal = sin(poloidalAngle);
            
            // Position - y-axis goes through the torus hole
            float x = (r1 + r2 * cosPoloidal) * cosToroidal;
            float y = r2 * sinPoloidal;
            float z = (r1 + r2 * cosPoloidal) * sinToroidal;
            
            // Normal (for flat shading, we'll calculate per face)
            float nx = cosPoloidal * cosToroidal;
            float ny = sinPoloidal;
            float nz = cosPoloidal * sinToroidal;
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }
    
    // Generate indices
    for (int i = 0; i < toroidalSegments; i++) {
        for (int j = 0; j < poloidalSegments; j++) {
            int current = i * (poloidalSegments + 1) + j;
            int next = i * (poloidalSegments + 1) + (j + 1);
            int currentNext = (i + 1) * (poloidalSegments + 1) + j;
            int nextNext = (i + 1) * (poloidalSegments + 1) + (j + 1);
            
            // First triangle
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(currentNext);
            
            // Second triangle
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currentNext);
        }
    }
}

TorusBuffers create_torus_buffers(wgpu::Device& device, float r1, float r2, int toroidalSegments, int poloidalSegments) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/torus.wgsl");
    if (!shaderModule) {
        std::cerr << "Failed to create torus structure shader module" << std::endl;
        exit(1);
    }

    TorusBuffers buf = {};

    // Generate torus vertices and indices
    std::vector<glm::f32> vertices;
    generate_torus_vertices(r1, r2, toroidalSegments, poloidalSegments, vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Torus Structure Vertex Buffer",
        .size = vertices.size() * sizeof(glm::f32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Torus Structure Index Buffer",
        .size = buf.indices.size() * sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .mappedAtCreation = false
    };
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Torus Structure Uniform Buffer",
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
        .label = "Torus Structure Render Pipeline",
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
        .label = "Torus Structure Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusStructureBuf, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix - y-axis already goes through the torus hole

    UniformData uniformData = {
        .model = model,
        .view = view,
        .projection = projection,
        .padding = {0.0f, 0.0f, 0.0f, 0.0f}
    };
    
    device.GetQueue().WriteBuffer(torusStructureBuf.uniformBuffer, 0, &uniformData, sizeof(UniformData));

    // Set the pipeline and bind group
    pass.SetPipeline(torusStructureBuf.pipeline);
    pass.SetBindGroup(0, torusStructureBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, torusStructureBuf.vertexBuffer, 0, torusStructureBuf.vertexBuffer.GetSize());
    pass.SetIndexBuffer(torusStructureBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, torusStructureBuf.indexBuffer.GetSize());

    // Draw the torus structure
    pass.DrawIndexed(torusStructureBuf.indices.size(), 1, 0, 0, 0);
} 