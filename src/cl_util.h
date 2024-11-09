#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120

#include <vector>
#include <OpenCL/cl.hpp>
#include <OpenCL/cl_gl.h>

typedef struct CLState {
    cl::Context* context = nullptr;
    std::vector<cl::Device> devices;
    cl::Device* selectedDevice = nullptr;
    cl::CommandQueue* queue = nullptr;
} CLState;

CLState* init_opencl();
cl::Program build_kernel(CLState* clState, std::string kernelPath);