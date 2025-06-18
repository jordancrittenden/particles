#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include "torus_dawn.h"
#include "ring_dawn.h"

// Set up transformation matrix for each circle
glm::mat4 get_coil_model_matrix(float angle, float r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

TorusBuffers create_torus_buffers(wgpu::Device& device, const Ring& ring, int numRings) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/torus.wgsl");

    TorusBuffers buf = {};

    // Generate solenoid vertices and indices using the ring vertex generation function
    std::vector<glm::f32> vertices;
    generate_ring_vertices(ring, vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Torus Vertex Buffer",
        .size = vertices.size() * sizeof(glm::f32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Torus Index Buffer",
        .size = buf.indices.size() * sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .mappedAtCreation = false
    };
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Torus Uniform Buffer",
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
        .label = "Torus Render Pipeline",
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
        .size = sizeof(glm::mat4) * 3
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .label = "Torus Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusBuf, int nCoils, float primaryRadius, glm::mat4 view, glm::mat4 projection) {
    // Draw each circle in the torus
    for (int i = 0; i < nCoils; ++i) {
        float angle = (2.0f * M_PI * i) / nCoils;
        glm::mat4 model = get_coil_model_matrix(angle, primaryRadius);
        std::vector<glm::mat4> matrices = {model, view, projection};
        device.GetQueue().WriteBuffer(torusBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 3);

        // Set the pipeline and bind group
        pass.SetPipeline(torusBuf.pipeline);
        pass.SetBindGroup(0, torusBuf.bindGroup);

        // Set the vertex and index buffers
        pass.SetVertexBuffer(0, torusBuf.vertexBuffer, 0, torusBuf.vertexBuffer.GetSize());
        pass.SetIndexBuffer(torusBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, torusBuf.indexBuffer.GetSize());

        // Draw the torus
        pass.DrawIndexed(torusBuf.indices.size(), 1, 0, 0, 0);
    }
} 