#include <iostream>
#include <fstream>
#include <OpenGL/OpenGL.h>
#include "cl_util.h"

void cl_exit_if_err(cl_int err, const char* msg) {
    if (err != CL_SUCCESS) {
        std::cerr << msg << ": " << get_cl_err_string(err) << std::endl;
        exit(1);
    }
}

CLState* init_opencl() {
    CLState* state = new CLState();

    // Obtain the current OpenGL CGL sharegroup
    CGLContextObj cglContext = CGLGetCurrentContext();
    CGLShareGroupObj shareGroup = CGLGetShareGroup(cglContext);

    if (!shareGroup) {
        std::cerr << "Failed to get CGL share group" << std::endl;
        return nullptr;
    }

    // Initialize OpenCL
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) {
        std::cerr << "No OpenCL platforms found" << std::endl;
        return nullptr;
    }

    // Choose the first platform
    cl::Platform platform = platforms.front();

    cl_int versionErr;
    std::string version = platform.getInfo<CL_PLATFORM_VERSION>(&versionErr);
    std::cout << "OpenCL Version: " << version << std::endl;
        
    platform.getDevices(CL_DEVICE_TYPE_GPU, &state->devices);
    if (state->devices.empty()) {
        std::cerr << "No GPU devices found for the chosen platform" << std::endl;
        return nullptr;
    }

    // Choose the first device
    state->selectedDevice = &state->devices.front();
    std::cout << "Using OpenCL device: " << state->selectedDevice->getInfo<CL_DEVICE_NAME>() << std::endl;

    // Check for OpenGL-OpenCL sharing support
    std::string extensions = state->selectedDevice->getInfo<CL_DEVICE_EXTENSIONS>();
    if (extensions.find("cl_APPLE_gl_sharing") == std::string::npos) {
        std::cerr << "OpenCL-OpenGL sharing is not supported on this device" << std::endl;
        return nullptr;
    }

    // Set up OpenCL context properties for OpenGL sharing
    cl_context_properties properties[] = {
#ifdef _WIN32
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(),
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
#elif defined(__APPLE__)
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)shareGroup,
#else // Linux
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(),
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
#endif
        0
    };

    // Create OpenCL context with OpenGL sharing enabled
    cl_int err;
    state->context = new cl::Context(state->devices, properties, nullptr, nullptr, &err);
    cl_exit_if_err(err, "Failed to create OpenCL context with OpenGL sharing");

     // Create OpenCL command queue
    state->queue = new cl::CommandQueue(*state->context, *state->selectedDevice, 0, &err);
    cl_exit_if_err(err, "Failed to create OpenCL command queue");

    std::cout << "OpenCL-OpenGL interop context created successfully." << std::endl;

    return state;
}

cl::Program build_kernel(CLState* clState, std::string kernelPath) {
    // Load kernel source
    std::ifstream kernelFile(kernelPath);
    std::string src(std::istreambuf_iterator<char>(kernelFile), (std::istreambuf_iterator<char>()));
    cl::Program program(*clState->context, src);
    cl_int buildErr = program.build(clState->devices);
    
    if (buildErr != CL_SUCCESS) {
        // Retrieve and print the error log
        std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*clState->selectedDevice);
        std::cerr << "Build failed with errors:\n" << buildLog << std::endl;
    } else {
        std::cout << "Program built successfully!" << std::endl;
    }

    return program;
}

const char* get_cl_err_string(cl_int err) {
    switch (err) {
        // run-time and JIT compiler errors
        case 0: return "CL_SUCCESS";
        case -1: return "CL_DEVICE_NOT_FOUND";
        case -2: return "CL_DEVICE_NOT_AVAILABLE";
        case -3: return "CL_COMPILER_NOT_AVAILABLE";
        case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case -5: return "CL_OUT_OF_RESOURCES";
        case -6: return "CL_OUT_OF_HOST_MEMORY";
        case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case -8: return "CL_MEM_COPY_OVERLAP";
        case -9: return "CL_IMAGE_FORMAT_MISMATCH";
        case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case -11: return "CL_BUILD_PROGRAM_FAILURE";
        case -12: return "CL_MAP_FAILURE";
        case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case -15: return "CL_COMPILE_PROGRAM_FAILURE";
        case -16: return "CL_LINKER_NOT_AVAILABLE";
        case -17: return "CL_LINK_PROGRAM_FAILURE";
        case -18: return "CL_DEVICE_PARTITION_FAILED";
        case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

        // compile-time errors
        case -30: return "CL_INVALID_VALUE";
        case -31: return "CL_INVALID_DEVICE_TYPE";
        case -32: return "CL_INVALID_PLATFORM";
        case -33: return "CL_INVALID_DEVICE";
        case -34: return "CL_INVALID_CONTEXT";
        case -35: return "CL_INVALID_QUEUE_PROPERTIES";
        case -36: return "CL_INVALID_COMMAND_QUEUE";
        case -37: return "CL_INVALID_HOST_PTR";
        case -38: return "CL_INVALID_MEM_OBJECT";
        case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case -40: return "CL_INVALID_IMAGE_SIZE";
        case -41: return "CL_INVALID_SAMPLER";
        case -42: return "CL_INVALID_BINARY";
        case -43: return "CL_INVALID_BUILD_OPTIONS";
        case -44: return "CL_INVALID_PROGRAM";
        case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
        case -46: return "CL_INVALID_KERNEL_NAME";
        case -47: return "CL_INVALID_KERNEL_DEFINITION";
        case -48: return "CL_INVALID_KERNEL";
        case -49: return "CL_INVALID_ARG_INDEX";
        case -50: return "CL_INVALID_ARG_VALUE";
        case -51: return "CL_INVALID_ARG_SIZE";
        case -52: return "CL_INVALID_KERNEL_ARGS";
        case -53: return "CL_INVALID_WORK_DIMENSION";
        case -54: return "CL_INVALID_WORK_GROUP_SIZE";
        case -55: return "CL_INVALID_WORK_ITEM_SIZE";
        case -56: return "CL_INVALID_GLOBAL_OFFSET";
        case -57: return "CL_INVALID_EVENT_WAIT_LIST";
        case -58: return "CL_INVALID_EVENT";
        case -59: return "CL_INVALID_OPERATION";
        case -60: return "CL_INVALID_GL_OBJECT";
        case -61: return "CL_INVALID_BUFFER_SIZE";
        case -62: return "CL_INVALID_MIP_LEVEL";
        case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
        case -64: return "CL_INVALID_PROPERTY";
        case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
        case -66: return "CL_INVALID_COMPILER_OPTIONS";
        case -67: return "CL_INVALID_LINKER_OPTIONS";
        case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

        // extension errors
        case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
        case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
        case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
        case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
        case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
        case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
        default: return "Unknown OpenCL error";
    }
}