#include "util/cl_util.h"
#include "current_segment.h"

cl::Buffer get_current_segment_buffer(cl::Context* context, const std::vector<CurrentVector>& currents) {
    std::vector<cl_float4> unrolled;
    for (auto& cur : currents) {
        unrolled.push_back(cl_float4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        unrolled.push_back(cl_float4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        unrolled.push_back(cl_float4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    return cl::Buffer(*context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * unrolled.size(), unrolled.data());
}