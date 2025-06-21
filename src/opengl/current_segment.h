#pragma once

#include <glm/glm.hpp>
#include "util/cl_util.h"

typedef struct CurrentVector {
    glm::f32vec4 x;  // position of the current in space
    glm::f32vec4 dx; // direction of the current in space
    float i;         // current in A
} CurrentVector;

cl::Buffer get_current_segment_buffer(cl::Context* context, const std::vector<CurrentVector>& currents);

void update_currents_buffer(cl::CommandQueue* queue, cl::Buffer& currentSegmentBufCL, const std::vector<CurrentVector>& currents);