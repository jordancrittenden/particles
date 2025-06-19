#include <dawn/webgpu_cpp.h>
#include <glm/gtc/type_ptr.hpp>
#include "particles_dawn.h"
#include "../util/wgpu_util.h"
#include "physical_constants.h"

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::vec4()> posF,
    std::function<glm::vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    int initialParticles,
    int maxParticles) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/particles.wgsl");

    ParticleBuffers buf = {};

    std::vector<glm::vec4> position_and_type;
    std::vector<glm::vec4> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < maxParticles; ++i) {
        if (i < initialParticles) {
            PARTICLE_SPECIES species = speciesF();
            glm::vec4 pos = posF();
            glm::vec4 vel = velF(species);

            pos[3] = (float)species;

            // [x, y, z, species]
            position_and_type.push_back(pos);
            // [dx, dy, dz, unused]
            velocity.push_back(vel);
        } else {
            // Placeholders for future particles that may be created via collisions

            // [x, y, z, species]
            position_and_type.push_back(glm::vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
            // [dx, dy, dz, unused]
            velocity.push_back(glm::vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
        }
    }

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Particle Vertex Buffer",
        .size = position_and_type.size() * sizeof(glm::vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, position_and_type.data(), position_and_type.size() * sizeof(glm::vec4));

    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Particle Uniform Buffer",
        .size = sizeof(glm::mat4) * 2,  // view, projection
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
            .minBindingSize = sizeof(glm::mat4) * 2
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
        { // Position
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        },
        { // Species
            .format = wgpu::VertexFormat::Float32,
            .offset = 3 * sizeof(float),
            .shaderLocation = 1
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(glm::vec4),
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
        .label = "Particle Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = shaderModule,
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
    buf.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {
        .binding = 0,
        .buffer = buf.uniformBuffer,
        .offset = 0,
        .size = sizeof(glm::mat4) * 2
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .layout = buf.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_particles(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const ParticleBuffers& particleBuf, int nParticles, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with matrices
    std::vector<glm::mat4> matrices = {view, projection};
    device.GetQueue().WriteBuffer(particleBuf.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 2);

    // Set the pipeline and bind group
    pass.SetPipeline(particleBuf.pipeline);
    pass.SetBindGroup(0, particleBuf.bindGroup);

    // Set the vertex buffer
    pass.SetVertexBuffer(0, particleBuf.vertexBuffer, 0, nParticles * sizeof(glm::vec4));

    // Draw the particles
    pass.Draw(nParticles, 1, 0, 0);  // nParticles vertices, 1 instance, starting at vertex 0, instance 0
}
