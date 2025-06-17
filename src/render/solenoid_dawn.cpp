#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include "solenoid_dawn.h"
#include "ring_dawn.h"

SolenoidBuffers create_solenoid_buffers(wgpu::Device& device, const Ring& ring) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/solenoid.wgsl");

    SolenoidBuffers buf = {};

    // Generate solenoid vertices and indices using the ring vertex generation function
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
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 6 * sizeof(float);
    vertexBufferLayout.attributeCount = attributes.size();
    vertexBufferLayout.attributes = attributes.data();

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
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
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

void render_solenoid(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const SolenoidBuffers& solenoidBuf, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    glm::mat4 model = glm::mat4(1.0f);
    std::vector<glm::mat4> matrices = {model, view, projection};
    device.GetQueue().WriteBuffer(solenoidBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 3);

    // Set the pipeline and bind group
    pass.SetPipeline(solenoidBuf.pipeline);
    pass.SetBindGroup(0, solenoidBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, solenoidBuf.vertexBuffer, 0, 6 * sizeof(float) * (solenoidBuf.indices.size() / 3 * 2));
    pass.SetIndexBuffer(solenoidBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, solenoidBuf.indices.size() * sizeof(unsigned int));

    // Draw the solenoid
    pass.DrawIndexed(solenoidBuf.indices.size(), 1, 0, 0, 0);
} 