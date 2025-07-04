#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include "util/wgpu_util.h"
#include "cell_box.h"

struct UniformData {
    glm::mat4 view;
    glm::mat4 projection;
};

// Generate wireframe box vertices (12 lines for a unit cube)
void generate_wireframe_box_vertices(std::vector<glm::f32>& vertices, std::vector<unsigned int>& indices) {
    // Unit cube vertices (position only, no normals needed for wireframe)
    vertices = {
        // Bottom face vertices
        -0.5f, -0.5f, -0.5f,  // 0
         0.5f, -0.5f, -0.5f,  // 1
         0.5f, -0.5f,  0.5f,  // 2
        -0.5f, -0.5f,  0.5f,  // 3
        
        // Top face vertices
        -0.5f,  0.5f, -0.5f,  // 4
         0.5f,  0.5f, -0.5f,  // 5
         0.5f,  0.5f,  0.5f,  // 6
        -0.5f,  0.5f,  0.5f   // 7
    };
    
    // Indices for 12 lines (2 indices per line)
    indices = {
        // Bottom face
        0, 1,  1, 2,  2, 3,  3, 0,
        // Top face
        4, 5,  5, 6,  6, 7,  7, 4,
        // Vertical edges
        0, 4,  1, 5,  2, 6,  3, 7
    };
}

CellBoxBuffers create_cell_box_buffers(wgpu::Device& device, const std::vector<Cell>& cells) {
    wgpu::ShaderModule shaderModule = create_shader_module(device, "shader/cell_box.wgsl");
    if (!shaderModule) {
        std::cerr << "Failed to create cell box shader module" << std::endl;
        exit(1);
    }

    CellBoxBuffers buf = {};
    buf.nCells = static_cast<glm::u32>(cells.size());

    // Generate wireframe box vertices and indices
    std::vector<glm::f32> vertices;
    generate_wireframe_box_vertices(vertices, buf.indices);

    // Create vertex buffer
    wgpu::BufferDescriptor vertexBufferDesc = {
        .label = "Cell Box Vertex Buffer",
        .size = vertices.size() * sizeof(glm::f32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.vertexBuffer = device.CreateBuffer(&vertexBufferDesc);
    device.GetQueue().WriteBuffer(buf.vertexBuffer, 0, vertices.data(), vertexBufferDesc.size);

    // Create index buffer
    wgpu::BufferDescriptor indexBufferDesc = {
        .label = "Cell Box Index Buffer",
        .size = buf.indices.size() * sizeof(glm::u32),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .mappedAtCreation = false
    };
    buf.indexBuffer = device.CreateBuffer(&indexBufferDesc);
    device.GetQueue().WriteBuffer(buf.indexBuffer, 0, buf.indices.data(), indexBufferDesc.size);

    // Create instance buffer for cell data (position only)
    // Each instance contains: [centerX, centerY, centerZ, padding]
    wgpu::BufferDescriptor instanceBufferDesc = {
        .label = "Cell Box Instance Buffer",
        .size = sizeof(glm::f32vec4) * buf.nCells, // 1 vec4 per instance
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.instanceBuffer = device.CreateBuffer(&instanceBufferDesc);

    // Initialize instance buffer with cell data
    std::vector<glm::f32vec4> instanceData;
    instanceData.reserve(cells.size());
    
    for (const auto& cell : cells) {
        // Use cell position directly (all cells have the same center)
        instanceData.push_back(cell.pos);
    }
    
    // Write instance data to buffer
    device.GetQueue().WriteBuffer(buf.instanceBuffer, 0, instanceData.data(), instanceData.size() * sizeof(glm::f32vec4));

    // Create visibility buffer (separate buffer for visibility control)
    wgpu::BufferDescriptor visibilityBufferDesc = {
        .label = "Cell Box Visibility Buffer",
        .size = sizeof(glm::u32) * buf.nCells,
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.visibilityBuffer = device.CreateBuffer(&visibilityBufferDesc);

    // Create uniform buffer for view and projection matrices
    wgpu::BufferDescriptor uniformBufferDesc = {
        .label = "Cell Box Uniform Buffer",
        .size = sizeof(UniformData),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .mappedAtCreation = false
    };
    buf.uniformBuffer = device.CreateBuffer(&uniformBufferDesc);

    // Create bind group layout
    std::vector<wgpu::BindGroupLayoutEntry> bindings = {
        { // Uniform buffer
            .binding = 0,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer = {
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(UniformData)
            }
        }, { // Visibility buffer
            .binding = 1,
            .visibility = wgpu::ShaderStage::Vertex,
            .buffer = {
                .type = wgpu::BufferBindingType::ReadOnlyStorage,
                .minBindingSize = sizeof(glm::u32) * buf.nCells
            }
        }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .entryCount = bindings.size(),
        .entries = bindings.data()
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
        }
    };

    // Instance data (1 vec4 per instance)
    std::vector<wgpu::VertexAttribute> instanceAttributes = {
        { // Center position
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 0,
            .shaderLocation = 1
        }
    };

    wgpu::VertexBufferLayout vertexBufferLayout = {
        .arrayStride = 3 * sizeof(glm::f32),
        .attributeCount = vertexAttributes.size(),
        .attributes = vertexAttributes.data()
    };

    wgpu::VertexBufferLayout instanceBufferLayout = {
        .arrayStride = sizeof(glm::f32vec4),
        .attributeCount = instanceAttributes.size(),
        .attributes = instanceAttributes.data(),
        .stepMode = wgpu::VertexStepMode::Instance
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
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .format = wgpu::TextureFormat::Depth24Plus
    };

    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .label = "Cell Box Render Pipeline",
        .layout = buf.pipelineLayout,
        .vertex = {
            .module = shaderModule,
            .entryPoint = "vertexMain",
            .bufferCount = bufferLayouts.size(),
            .buffers = bufferLayouts.data()
        },
        .fragment = &fragmentState,
        .primitive = {
            .topology = wgpu::PrimitiveTopology::LineList,
            .frontFace = wgpu::FrontFace::CCW
        },
        .depthStencil = &depthStencilState
    };
    buf.pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create bind group
    std::vector<wgpu::BindGroupEntry> bindGroupEntries = {
        { // Uniform buffer
            .binding = 0,
            .buffer = buf.uniformBuffer,
            .offset = 0,
            .size = sizeof(UniformData)
        }, { // Visibility buffer
            .binding = 1,
            .buffer = buf.visibilityBuffer,
            .offset = 0,
            .size = sizeof(glm::u32) * buf.nCells
        }
    };

    wgpu::BindGroupDescriptor bindGroupDesc = {
        .label = "Cell Box Bind Group",
        .layout = buf.bindGroupLayout,
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data()
    };
    buf.bindGroup = device.CreateBindGroup(&bindGroupDesc);

    return buf;
}

void render_cell_boxes(wgpu::Device& device, wgpu::RenderPassEncoder& pass, const CellBoxBuffers& cellBoxBuf, glm::mat4 view, glm::mat4 projection) {
    UniformData uniformData = {
        .view = view,
        .projection = projection
    };
    
    // Update uniform buffer with view and projection matrices
    device.GetQueue().WriteBuffer(cellBoxBuf.uniformBuffer, 0, &uniformData, sizeof(UniformData));

    // Set the pipeline and bind group
    pass.SetPipeline(cellBoxBuf.pipeline);
    pass.SetBindGroup(0, cellBoxBuf.bindGroup);

    // Set the vertex and index buffers
    pass.SetVertexBuffer(0, cellBoxBuf.vertexBuffer, 0, cellBoxBuf.vertexBuffer.GetSize());
    pass.SetVertexBuffer(1, cellBoxBuf.instanceBuffer, 0, cellBoxBuf.instanceBuffer.GetSize());
    pass.SetIndexBuffer(cellBoxBuf.indexBuffer, wgpu::IndexFormat::Uint32, 0, cellBoxBuf.indexBuffer.GetSize());

    // Draw the cell boxes with instancing
    pass.DrawIndexed(cellBoxBuf.indices.size(), cellBoxBuf.nCells, 0, 0, 0);
}

void update_cell_visibility(wgpu::Device& device, const CellBoxBuffers& cellBoxBuf, const std::vector<bool>& visibility) {
    // Convert bool vector to u32 vector for GPU
    std::vector<glm::u32> visibilityData;
    visibilityData.reserve(visibility.size());
    for (bool visible : visibility) {
        visibilityData.push_back(visible ? 1u : 0u);
    }
    
    // Update visibility buffer
    device.GetQueue().WriteBuffer(cellBoxBuf.visibilityBuffer, 0, visibilityData.data(), visibilityData.size() * sizeof(glm::u32));
}
