#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120

#include <vector>
#include <opencl.hpp>
#include <OpenCL/cl_gl.h>

typedef struct CLState {
    cl::Context* context = nullptr;
    std::vector<cl::Device> devices;
    cl::Device* selectedDevice = nullptr;
    cl::CommandQueue* queue = nullptr;
} CLState;

CLState* init_opencl();
cl::Program build_kernel(CLState* clState, std::string kernelPath, std::string headerPath);
void cl_exit_if_err(cl_int err, const char* msg);
const char* get_cl_err_string(cl_int err);