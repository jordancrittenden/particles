#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <dawn/webgpu_cpp.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "fields.h"
#include "util/wgpu_util.h"

#define ARROW_TIP_SEGMENTS 10
#define ARROW_SHAFT_SEGMENTS 8

struct Uniforms {
    glm::mat4 view;
    glm::mat4 projection;
};

// Vector arrow geometry - points along the z-axis
std::vector<glm::f32> create_vector_geometry(float length) {
    std::vector<float> vertices;

    float tipWidth = length / 15.0f;
    float tipLength = length / 3.0f;
    float shaftWidth = tipWidth / 4.0f;

    // Arrow shaft - create thin cylinder using triangles
    for (int i = 0; i < ARROW_SHAFT_SEGMENTS; i++) {
        float angle1 = i * (2 * M_PI) / (float)ARROW_SHAFT_SEGMENTS;
        float angle2 = (i + 1) * (2 * M_PI) / (float)ARROW_SHAFT_SEGMENTS;
        
        glm::f32vec3 base1(shaftWidth * cos(angle1), shaftWidth * sin(angle1), 0.0f);
        glm::f32vec3 base2(shaftWidth * cos(angle2), shaftWidth * sin(angle2), 0.0f);
        glm::f32vec3 top1(shaftWidth * cos(angle1), shaftWidth * sin(angle1), length - tipLength);
        glm::f32vec3 top2(shaftWidth * cos(angle2), shaftWidth * sin(angle2), length - tipLength);
        
        // Two triangles per segment: base1, base2, top1 and base2, top2, top1
        vertices.insert(vertices.end(), {
            base1.x, base1.y, base1.z,
            base2.x, base2.y, base2.z,
            top1.x, top1.y, top1.z
        });
        vertices.insert(vertices.end(), {
            base2.x, base2.y, base2.z,
            top2.x, top2.y, top2.z,
            top1.x, top1.y, top1.z
        });
    }

    // Arrow tip - convert triangle fan to triangle list
    // Center vertex (tip)
    glm::f32vec3 center(0.0f, 0.0f, length);
    
    // Create triangles: center + base vertices
    for (int i = 0; i < ARROW_TIP_SEGMENTS; i++) {
        float angle1 = i * (2 * M_PI) / (float)ARROW_TIP_SEGMENTS;
        float angle2 = (i + 1) * (2 * M_PI) / (float)ARROW_TIP_SEGMENTS;
        
        glm::f32vec3 base1(tipWidth * cos(angle1), tipWidth * sin(angle1), length - tipLength);
        glm::f32vec3 base2(tipWidth * cos(angle2), tipWidth * sin(angle2), length - tipLength);
        
        // Triangle: center, base1, base2
        vertices.insert(vertices.end(), {
            center.x, center.y, center.z,
            base1.x, base1.y, base1.z,
            base2.x, base2.y, base2.z
        });
    }

    return vertices;
}

FieldRender create_fields_render(wgpu::Device& device, std::vector<glm::f32vec4>& loc, std::vector<glm::f32vec4>& vec, float length) {
    FieldRender render = {};

    // Create render shader module
    wgpu::ShaderModule renderShaderModule = create_shader_module(device, "shader/wgpu/vector.wgsl");
    if (!renderShaderModule) {
        std::cerr << "Failed to create render shader module" << std::endl;
        exit(1);
    }
    
    // Create vertex buffer for arrow geometry
    std::vector<glm::f32> vertices = create_vector_geometry(length);
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Field Vertex Buffer",
        .size = vertices.size() * sizeof(glm::f32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex
    };
    render.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(render.vertexBuffer, 0, vertices.data(), vertices.size() * sizeof(glm::f32));
    
    // Create instance buffers
    wgpu::BufferDescriptor instanceBufferDesc = {
        .label = "Field Instance Buffer",
        .size = loc.size() * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex
    };
    render.instanceBuffer = device.CreateBuffer(&instanceBufferDesc);
    device.GetQueue().WriteBuffer(render.instanceBuffer, 0, loc.data(), loc.size() * sizeof(glm::f32vec4));
    
    // Create uniform buffer
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Field Uniform Buffer",
        .size = sizeof(Uniforms),
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst
    };
    render.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);
    
    // Create render bind group layout
    wgpu::BindGroupLayoutEntry renderBinding = {
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
        .buffer = {
            .type = wgpu::BufferBindingType::Uniform,
            .minBindingSize = sizeof(Uniforms)
        }
    };
    
    wgpu::BindGroupLayoutDescriptor renderBindGroupLayoutDesc = {
        .label = "Field Bind Group Layout",
        .entryCount = 1,
        .entries = &renderBinding
    };
    render.bindGroupLayout = device.CreateBindGroupLayout(&renderBindGroupLayoutDesc);
    
    // Create render pipeline layout
    wgpu::PipelineLayoutDescriptor renderPipelineLayoutDesc = {
        .label = "Field Pipeline Layout",
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &render.bindGroupLayout
    };
    render.pipelineLayout = device.CreatePipelineLayout(&renderPipelineLayoutDesc);
        
    wgpu::VertexAttribute vertexAttribute = {
        .format = wgpu::VertexFormat::Float32x3,
        .offset = 0,
        .shaderLocation = 0
    };
    
    wgpu::VertexAttribute locationAttribute = {
        .format = wgpu::VertexFormat::Float32x4,
        .offset = 0,
        .shaderLocation = 1
    };
    
    wgpu::VertexAttribute fieldAttribute = {
        .format = wgpu::VertexFormat::Float32x4,
        .offset = 0,
        .shaderLocation = 2
    };
    
    std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts = {
        { // Vertex buffer layout
            .arrayStride = sizeof(glm::f32vec3),
            .attributeCount = 1,
            .attributes = &vertexAttribute
        }, { // Instance buffer layout
            .arrayStride = sizeof(glm::f32vec4),
            .attributeCount = 1,
            .attributes = &locationAttribute,
            .stepMode = wgpu::VertexStepMode::Instance
        }, { // Field buffer layout
            .arrayStride = sizeof(glm::f32vec4),
            .attributeCount = 1,
            .attributes = &fieldAttribute,
            .stepMode = wgpu::VertexStepMode::Instance
        }
    };
    
    // Create color target state
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
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .format = wgpu::TextureFormat::Depth24Plus
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor renderPipelineDesc = {
        .label = "Field Render Pipeline",
        .layout = render.pipelineLayout,
        .vertex = {
            .module = renderShaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = vertexBufferLayouts.size(),
            .buffers = vertexBufferLayouts.data()
        },
        .fragment = &fragmentState,
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList
        },
        .depthStencil = &depthStencilState
    };
    render.pipeline = device.CreateRenderPipeline(&renderPipelineDesc);
    
    // Create render bind group
    wgpu::BindGroupEntry renderBindGroupEntry = {
        .binding = 0,
        .buffer = render.uniformBuffer,
        .offset = 0,
        .size = sizeof(Uniforms)
    };
    
    wgpu::BindGroupDescriptor renderBindGroupDesc = {
        .layout = render.bindGroupLayout,
        .entryCount = 1,
        .entries = &renderBindGroupEntry
    };
    render.bindGroup = device.CreateBindGroup(&renderBindGroupDesc);

    return render;
}

void render_fields(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const FieldRender& fieldRender, wgpu::Buffer& fieldBuffer, int numFieldVectors, glm::mat4 view, glm::mat4 projection) {
    // Update uniform buffer with view and projection matrices
    Uniforms uniforms = {
        .view = view,
        .projection = projection
    };
    
    device.GetQueue().WriteBuffer(fieldRender.uniformBuffer, 0, &uniforms, sizeof(Uniforms));
    
    // Set pipeline and bind group
    pass.SetPipeline(fieldRender.pipeline);
    pass.SetBindGroup(0, fieldRender.bindGroup);
    
    // Set vertex buffers
    pass.SetVertexBuffer(0, fieldRender.vertexBuffer);
    pass.SetVertexBuffer(1, fieldRender.instanceBuffer);
    pass.SetVertexBuffer(2, fieldBuffer);
    
    // Draw all triangles (shaft + tip)
    // Shaft: ARROW_SHAFT_SEGMENTS * 2 triangles * 3 vertices = 16 * 3 = 48 vertices
    // Tip: ARROW_TIP_SEGMENTS triangles * 3 vertices = 10 * 3 = 30 vertices
    // Total: 48 + 30 = 78 vertices
    pass.Draw(ARROW_SHAFT_SEGMENTS * 2 * 3 + ARROW_TIP_SEGMENTS * 3, numFieldVectors, 0, 0);
}