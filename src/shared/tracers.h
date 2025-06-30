#pragma once

#include <webgpu/webgpu_cpp.h>
#include <functional>
#include <glm/glm.hpp>
#include "physical_constants.h"
#include "cell.h"

#define TRACER_LENGTH 500

struct TracerBuffers {
    // tracers: [
    //    trace0_x0, trace0_y0, trace0_z0, unused, trace0_x1, trace0_y1, trace0_z1, unused, ..., trace0_xEND, trace0_yEND, trace0_zEND, unused,
    //    trace1_x0, trace1_y0, trace1_z0, unused, trace1_x1, trace1_y1, trace1_z1, unused, ..., trace1_xEND, trace1_yEND, trace1_zEND, unused,
    //    ...
    //    traceN_x0, traceN_y0, traceN_z0, unused, traceN_x1, traceN_y1, traceN_z1, unused, ..., traceN_xEND, traceN_yEND, traceN_zEND, unused,
    // ]
    wgpu::Buffer e_traces;    // Electric field traces
    wgpu::Buffer b_traces;    // Magnetic field traces
    glm::u32 nTracers;
};

TracerBuffers create_tracer_buffers(wgpu::Device& device, const std::vector<glm::f32vec4>& loc);