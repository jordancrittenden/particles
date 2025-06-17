#include <dawn/webgpu_cpp.h>
#include <glm/gtc/type_ptr.hpp>
#include "axes_dawn.h"
#include "../util/wgpu_util.h"
#include "physical_constants.h"

AxesBuffers create_axes_buffers(wgpu::Device& device) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/axes.wgsl");

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

    wgpu::BufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.size = sizeof(axisVertices);
    vertexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    vertexBufferDesc.mappedAtCreation = false;
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, axisVertices, sizeof(axisVertices));

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
    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineList;
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
