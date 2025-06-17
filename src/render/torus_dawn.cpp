#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include "torus_dawn.h"
#include "ring_dawn.h"

TorusBuffers create_torus_buffers(wgpu::Device& device, const Ring& ring, int numRings) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/torus.wgsl");

    TorusBuffers buf = {};
    buf.numRings = numRings;

    // Generate vertices for a single ring
    std::vector<float> vertices;
    generate_ring_vertices(ring, vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.size = vertices.size() * sizeof(float);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {};
    indexBufferDesc.size = buf.indices.size() * sizeof(unsigned int);
    indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    indexBufferDesc.mappedAtCreation = false;
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create instance buffer for ring positions
    std::vector<float> instanceData;
    float ringSpacing = ring.d / (numRings - 1);
    for (int i = 0; i < numRings; ++i) {
        float z = -ring.d / 2.0f + i * ringSpacing;
        instanceData.push_back(z);
    }

    wgpu::BufferDescriptor instanceBufferDesc = {};
    instanceBufferDesc.size = instanceData.size() * sizeof(float);
    instanceBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    instanceBufferDesc.mappedAtCreation = false;
    buf.instanceBuffer = device.CreateBuffer(&instanceBufferDesc);
    device.GetQueue().WriteBuffer(buf.instanceBuffer, 0, instanceData.data(), instanceBufferDesc.size);

    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {};
    uniformBufferDesc.size = sizeof(glm::mat4) * 3;  // model, view, projection
    uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    uniformBufferDesc.mappedAtCreation = false;
    buf.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create bind group layout
    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    binding.visibility = wgpu::ShaderStage::Vertex;
    binding.buffer.type = wgpu::BufferBindingType::Uniform;
    binding.buffer.minBindingSize = sizeof(glm::mat4) * 3;

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &binding;
    buf.bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Create pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &buf.bindGroupLayout;
    buf.pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create vertex state
    std::vector<wgpu::VertexAttribute> attributes = {
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        },
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1
        },
        {
            .format = wgpu::VertexFormat::Float32,
            .offset = 0,
            .shaderLocation = 2
        }
    };

    std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts = {
        {
            .arrayStride = 6 * sizeof(float),
            .stepMode = wgpu::VertexStepMode::Vertex,
            .attributeCount = 2,
            .attributes = attributes.data()
        },
        {
            .arrayStride = sizeof(float),
            .stepMode = wgpu::VertexStepMode::Instance,
            .attributeCount = 1,
            .attributes = &attributes[2]
        }
    };

    // Create color state
    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = wgpu::TextureFormat::BGRA8Unorm;
    colorTarget.blend = nullptr;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    // Create fragment state
    wgpu::FragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fragmentMain";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = buf.pipelineLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vertexMain";
    pipelineDesc.vertex.bufferCount = vertexBufferLayouts.size();
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    buf.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = buf.uniformBuffer;
    bindGroupEntry.offset = 0;
    bindGroupEntry.size = sizeof(glm::mat4) * 3;

    wgpu::BindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = buf.bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &bindGroupEntry;
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_torus(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const TorusBuffers& torusBuf, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    glm::mat4 model = glm::mat4(1.0f);
    std::vector<glm::mat4> matrices = {model, view, projection};
    device.GetQueue().WriteBuffer(torusBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 3);

    // Set the pipeline and bind group
    pass.SetPipeline(torusBuf.pipeline);
    pass.SetBindGroup(0, torusBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, torusBuf.vertexBuffer, 0, 6 * sizeof(float) * (torusBuf.indices.size() / 3 * 2));
    pass.SetVertexBuffer(1, torusBuf.instanceBuffer, 0, sizeof(float) * torusBuf.numRings);
    pass.SetIndexBuffer(torusBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, torusBuf.indices.size() * sizeof(unsigned int));

    // Draw the torus using instancing
    pass.DrawIndexed(torusBuf.indices.size(), torusBuf.numRings, 0, 0, 0);
} 