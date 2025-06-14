#pragma once

#include <glm/glm.hpp>
#include "util/cl_util.h"

typedef struct CurrentVector {
    glm::vec4 x;  // position of the current in space
    glm::vec4 dx; // direction of the current in space
    float i;      // current in A
} CurrentVector;

cl::Buffer get_current_segment_buffer(cl::Context* context, const std::vector<CurrentVector>& currents);