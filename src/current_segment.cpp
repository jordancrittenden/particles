#include <vector>
#include <glm/glm.hpp>
#include "current_segment.h"

std::vector<glm::f32vec4> unroll_currents(const std::vector<CurrentVector>& currents) {
    std::vector<glm::f32vec4> unrolled;
    for (auto& cur : currents) {
        unrolled.push_back(glm::f32vec4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        unrolled.push_back(glm::f32vec4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        unrolled.push_back(glm::f32vec4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    return unrolled;
}

wgpu::Buffer get_current_segment_buffer(wgpu::Device& device, const std::vector<CurrentVector>& currents) {
    std::vector<glm::f32vec4> unrolled = unroll_currents(currents);
    wgpu::BufferDescriptor currentSegmentsBufferDesc = {
        .label = "Current Segments Buffer",
        .size = unrolled.size() * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    wgpu::Buffer currentSegmentsBuffer = device.CreateBuffer(&currentSegmentsBufferDesc);
    device.GetQueue().WriteBuffer(currentSegmentsBuffer, 0, unrolled.data(), unrolled.size() * sizeof(glm::f32vec4));

    return currentSegmentsBuffer;
}

void update_currents_buffer(wgpu::Device& device, wgpu::Buffer& currentSegmentsBuffer, const std::vector<CurrentVector>& currents) {
    std::vector<glm::f32vec4> unrolled = unroll_currents(currents);
    device.GetQueue().WriteBuffer(currentSegmentsBuffer, 0, unrolled.data(), unrolled.size() * sizeof(glm::f32vec4));
}