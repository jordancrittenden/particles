#include "util/cl_util.h"
#include "current_segment.h"

std::vector<cl_float4> unroll_currents(const std::vector<CurrentVector>& currents) {
    std::vector<cl_float4> unrolled;
    for (auto& cur : currents) {
        unrolled.push_back(cl_float4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        unrolled.push_back(cl_float4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        unrolled.push_back(cl_float4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    return unrolled;
}

cl::Buffer get_current_segment_buffer(cl::Context* context, const std::vector<CurrentVector>& currents) {
    std::vector<cl_float4> unrolled = unroll_currents(currents);
    return cl::Buffer(*context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * unrolled.size(), unrolled.data());
}

void update_currents_buffer(cl::CommandQueue* queue, cl::Buffer& currentSegmentBufCL, const std::vector<CurrentVector>& currents) {
    std::vector<cl_float4> unrolled = unroll_currents(currents);
    queue->enqueueWriteBuffer(currentSegmentBufCL, CL_TRUE, 0, sizeof(cl_float4) * unrolled.size(), unrolled.data());
}