#include "shared/tracers.h"
#include <iostream>

TracerBuffers create_tracer_buffers(wgpu::Device& device, const std::vector<glm::f32vec4>& loc) {
    TracerBuffers buffers = {};
    
    // Trace geometry - points along the x-axis
    std::vector<glm::f32vec4> tracerTrails;
    float sep = 0.01f * _M;
    for (auto& pos : loc) {
        for (int i = 0; i < TRACER_LENGTH; i++) {
            tracerTrails.push_back(glm::f32vec4 { pos.x, pos.y, pos.z, 0.0f });
        }
    }

    // Create E field tracer buffer
    wgpu::BufferDescriptor eBufferDesc = {
        .label = "E Tracer Buffer",
        .size = tracerTrails.size() * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buffers.e_traces = device.CreateBuffer(&eBufferDesc);
    device.GetQueue().WriteBuffer(buffers.e_traces, 0, tracerTrails.data(), tracerTrails.size() * sizeof(glm::f32vec4));
    
    // Create B field tracer buffer
    wgpu::BufferDescriptor bBufferDesc = {
        .label = "B Tracer Buffer",
        .size = tracerTrails.size() * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buffers.b_traces = device.CreateBuffer(&bBufferDesc);
    device.GetQueue().WriteBuffer(buffers.b_traces, 0, tracerTrails.data(), tracerTrails.size() * sizeof(glm::f32vec4));

    buffers.nTracers = loc.size();

    return buffers;
}
