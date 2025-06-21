#pragma once

#include <glm/glm.hpp>
#include "util/wgpu_util.h"

typedef struct CurrentVector {
    glm::f32vec4 x;  // position of the current in space
    glm::f32vec4 dx; // direction of the current in space
    float i;         // current in A
} CurrentVector;

wgpu::Buffer get_current_segment_buffer(wgpu::Device& device, const std::vector<CurrentVector>& currents);

void update_currents_buffer(wgpu::Device& device, wgpu::Buffer& currentSegmentBufCL, const std::vector<CurrentVector>& currents);