#include <iostream>
#include <fstream>
#include <OpenGL/OpenGL.h>
#include "cl_util.h"

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
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL context with OpenGL sharing: " << err << std::endl;
        return nullptr;
    }

     // Create OpenCL command queue
    state->queue = new cl::CommandQueue(*state->context, *state->selectedDevice, 0, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL command queue: " << err << std::endl;
        return nullptr;
    }

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