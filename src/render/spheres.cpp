#define _USE_MATH_DEFINES
#include <vector>
#include <glm/glm.hpp>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "render/spheres.h"
#include "util/wgpu_util.h"
#include "physical_constants.h"

std::pair<std::vector<glm::f32vec3>, std::vector<glm::u32>> create_sphere_vertices(glm::u32 numSamples, glm::f32 radius) {
    std::vector<glm::f32vec3> vertices;
    std::vector<glm::u32> indices;
    
    // Create sphere using latitude/longitude approach for simplicity
    const float pi = M_PI;
    const float twoPi = 2.0f * pi;
    
    for (glm::u32 lat = 0; lat <= numSamples; ++lat) {
        float theta = lat * pi / numSamples;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (glm::u32 lon = 0; lon <= numSamples; ++lon) {
            float phi = lon * twoPi / numSamples;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(glm::f32vec3(x * radius, y * radius, z * radius));
        }
    }
    
    // Generate indices for triangles
    for (glm::u32 lat = 0; lat < numSamples; ++lat) {
        for (glm::u32 lon = 0; lon < numSamples; ++lon) {
            glm::u32 current = lat * (numSamples + 1) + lon;
            glm::u32 next = current + numSamples + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    return {vertices, indices};
}

SphereRender create_sphere_render(wgpu::Device& device) {
    SphereRender render = {};
    
    // Create render shader module
    wgpu::ShaderModule renderShaderModule = create_shader_module(device, "shader/spheres.wgsl", {"kernel/physical_constants.wgsl"});
    if (!renderShaderModule) {
        std::cerr << "Failed to create sphere render shader module" << std::endl;
        exit(1);
    }

    // Create sphere vertices using 8 samples as requested
    auto [sphereVertices, sphereIndices] = create_sphere_vertices(8, 1.0f);
    
    // Create vertex buffer for sphere geometry
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Sphere Vertex Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = sphereVertices.size() * sizeof(glm::f32vec3),
        .mappedAtCreation = false
    };
    render.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(render.vertexBuffer, 0, sphereVertices.data(), vertexBufferDesc.size);

    // Create index buffer for sphere geometry
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Sphere Index Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = sphereIndices.size() * sizeof(glm::u32),
        .mappedAtCreation = false
    };
    render.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    render.indexCount = sphereIndices.size();
    device.GetQueue().WriteBuffer(render.indexBuffer, 0, sphereIndices.data(), indexBufferDesc.size);

    // Create uniform buffer for rendering
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Sphere Uniform Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(glm::mat4) * 2,  // view, projection
        .mappedAtCreation = false
    };
    render.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create render bind group layout
    wgpu::BindGroupLayoutEntry renderBinding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(glm::mat4) * 2
        }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .label = "Sphere Render Bind Group Layout",
        .entryCount = 1,
        .entries = &renderBinding
    };
    render.bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Create render pipeline layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {
        .label = "Sphere Render Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &render.bindGroupLayout
    };
    render.pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create vertex state for sphere geometry
    std::vector<wgpu::VertexAttribute> sphereAttributes = {
        {
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 0
        }
    };

    wgpu::VertexBufferLayout sphereBufferLayout = {
        .arrayStride = sizeof(glm::f32vec3),
        .attributeCount = sphereAttributes.size(),
        .attributes = sphereAttributes.data()
    };

    // Create instance buffer layout for particle positions
    std::vector<wgpu::VertexAttribute> instanceAttributes = {
        {
            .format = wgpu::VertexFormat::Float32x4,
            .offset = 0,
            .shaderLocation = 1
        }
    };

    wgpu::VertexBufferLayout instanceBufferLayout = {
        .stepMode = wgpu::VertexStepMode::Instance,
        .arrayStride = sizeof(glm::f32vec4),
        .attributeCount = static_cast<uint32_t>(instanceAttributes.size()),
        .attributes = instanceAttributes.data()
    };

    std::vector<wgpu::VertexBufferLayout> bufferLayouts = {sphereBufferLayout, instanceBufferLayout};

    // Create color state
    wgpu::ColorTargetState colorTarget = {
        .format = wgpu::TextureFormat::BGRA8Unorm,
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
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .label = "Sphere Render Pipeline",
        .layout = render.pipelineLayout,
        .vertex = {
            .module = renderShaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = bufferLayouts.size(),
            .buffers = bufferLayouts.data()
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList
        },
        .depthStencil = &depthStencilState,
        .fragment = &fragmentState
    };
    render.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create render bind group
    wgpu::BindGroupEntry bindGroupEntry = {
        .binding = 0,
        .buffer = render.uniformBuffer,
        .offset = 0,
        .size = sizeof(glm::mat4) * 2
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .layout = render.bindGroupLayout,
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    render.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return render;
}

void render_particles_as_spheres(
    wgpu::Device& device,
    wgpu::RenderPassEncoder& pass,
    const ParticleBuffers& particleBuf,
    const SphereRender& render,
    glm::u32 nParticles,
    glm::mat4 view,
    glm::mat4 projection) 
{
    // Update uniform buffer with matrices
    std::vector<glm::mat4> matrices = {view, projection};
    device.GetQueue().WriteBuffer(render.uniformBuffer, 0, matrices.data(), sizeof(glm::mat4) * 2);

    // Set the pipeline and bind group
    pass.SetPipeline(render.pipeline);
    pass.SetBindGroup(0, render.bindGroup);

    // Set the vertex buffer (sphere geometry)
    pass.SetVertexBuffer(0, render.vertexBuffer, 0, render.vertexBuffer.GetSize());

    // Set the index buffer
    pass.SetIndexBuffer(render.indexBuffer, wgpu::IndexFormat::Uint32, 0, render.indexBuffer.GetSize());

    // Set the instance buffer (particle positions)
    pass.SetVertexBuffer(1, particleBuf.pos, 0, nParticles * sizeof(glm::f32vec4));

    // Draw the spheres (indexed, nParticles instances)
    pass.DrawIndexed(render.indexCount, nParticles, 0, 0, 0);
}