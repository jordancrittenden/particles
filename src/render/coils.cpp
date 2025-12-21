#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include "util/wgpu_util.h"
#include "coils.h"
#include "ring.h"

struct UniformData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::f32 toroidalI;
    glm::f32 padding[3]; // Padding to align to 16-byte boundary
};

// Set up transformation matrix for each circle
glm::mat4 get_coil_model_matrix(glm::f32 angle, glm::f32 r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

CoilsBuffers create_coils_buffers(wgpu::Device& device, const Ring& ring, glm::u16 nCoils) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/coils.wgsl");
    if (!shaderModule) {
        std::cerr << "Failed to create coils shader module" << std::endl;
        exit(1);
    }

    CoilsBuffers buf = {};
    buf.nCoils = nCoils;

    // Generate solenoid vertices and indices using the ring vertex generation function
    std::vector<glm::f32> vertices;
    generate_ring_vertices(ring, vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Coils Vertex Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(glm::f32),
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Coils Index Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = buf.indices.size() * sizeof(glm::u32),
        .mappedAtCreation = false
    };
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create instance buffer for model matrices
    wgpu::BufferDescriptor instanceBufferDesc = {
        .label = "Coils Instance Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = sizeof(glm::mat4) * nCoils,
        .mappedAtCreation = false
    };
    buf.instanceBuffer = device.CreateBuffer(&instanceBufferDesc);

    // Create uniform buffer for view and projection matrices
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Coils Uniform Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(UniformData),
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

    // Create vertex state with two buffer layouts: vertex data and instance data
    std::vector<wgpu::VertexAttribute> vertexAttributes = {
        { // Position
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        }, { // Normal
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 3 * sizeof(glm::f32),
            .shaderLocation = 1
        }
    };

    // Model matrix (4 vec4s)
    std::vector<wgpu::VertexAttribute> instanceAttributes = {
        {
            .format = wgpu::VertexFormat::Float32x4,
            .offset = 0,
            .shaderLocation = 2
        }, {
            .format = wgpu::VertexFormat::Float32x4,
            .offset = 4 * sizeof(glm::f32),
            .shaderLocation = 3
        }, {
            .format = wgpu::VertexFormat::Float32x4,
            .offset = 8 * sizeof(glm::f32),
            .shaderLocation = 4
        }, {
            .format = wgpu::VertexFormat::Float32x4,
            .offset = 12 * sizeof(glm::f32),
            .shaderLocation = 5
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = 6 * sizeof(glm::f32),
        .attributeCount = vertexAttributes.size(),
        .attributes = vertexAttributes.data()
    };

    wgpu::VertexBufferLayout instanceBufferLayout = {
        .stepMode = wgpu::VertexStepMode::Instance,
        .arrayStride = sizeof(glm::mat4),
        .attributeCount = instanceAttributes.size(),
        .attributes = instanceAttributes.data()
    };

    std::vector<wgpu::VertexBufferLayout> bufferLayouts = {vertexBufferLayout, instanceBufferLayout};

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
        .format = wgpu::TextureFormat::Depth24Plus,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .label = "Coils Render Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = bufferLayouts.size(),
            .buffers = bufferLayouts.data()
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .frontFace = wgpu::FrontFace::CCW
        },
        .depthStencil = &depthStencilState,
        .fragment = &fragmentState
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
        .label = "Coils Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_coils(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const CoilsBuffers& coilsBuf, glm::f32 primaryRadius, glm::f32 toroidalI, glm::mat4 view, glm::mat4 projection) {
    // Generate model matrices for each ring
    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(coilsBuf.nCoils);
    
    for (glm::u16 i = 0; i < coilsBuf.nCoils; i++) {
        glm::f32 angle = (2.0f * M_PI * i) / coilsBuf.nCoils;
        glm::mat4 model = get_coil_model_matrix(angle, primaryRadius);
        modelMatrices.push_back(model);
    }
    
    // Update instance buffer with model matrices
    device.GetQueue().WriteBuffer(coilsBuf.instanceBuffer, 0, modelMatrices.data(), sizeof(glm::mat4) * coilsBuf.nCoils);
    
    UniformData uniformData = {
        .view = view,
        .projection = projection,
        .toroidalI = toroidalI,
        .padding = {0.0f, 0.0f, 0.0f}
    };
    
    // Update uniform buffer with view, projection, and toroidalI
    device.GetQueue().WriteBuffer(coilsBuf.uniformBuffer, 0, &uniformData, sizeof(UniformData));

    // Set the pipeline and bind group
    pass.SetPipeline(coilsBuf.pipeline);
    pass.SetBindGroup(0, coilsBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, coilsBuf.vertexBuffer, 0, coilsBuf.vertexBuffer.GetSize());
    pass.SetVertexBuffer(1, coilsBuf.instanceBuffer, 0, coilsBuf.instanceBuffer.GetSize());
    pass.SetIndexBuffer(coilsBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, coilsBuf.indexBuffer.GetSize());

    // Draw the torus with instancing
    pass.DrawIndexed(coilsBuf.indices.size(), coilsBuf.nCoils, 0, 0, 0);
} 