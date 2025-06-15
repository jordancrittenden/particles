#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/OpenGL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdlib>

#include "util/cl_util.h"
#include "util/gl_util.h"
#include "keyboard.h"
#include "args.h"
#include "state.h"
#include "scene.h"
#include "tokamak.h"
#include "current_segment.h"

// OpenGL window
int windowWidth = 1600;
int windowHeight = 1200;
GLFWwindow* window = nullptr;

// FPS
int targetFPS = 60;

// Main function
int main(int argc, char* argv[]) {
    TorusParameters torus;
    SolenoidParameters solenoid;
    SimulationState state;

    // Parse CLI arguments into state variables
    try {
        auto args = parse_args(argc, argv);
        extract_state_vars(args, &state, &windowWidth, &windowHeight, &targetFPS);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Inialize OpenGL and OpenCL
    float aspectRatio = (float)windowWidth / (float)windowHeight;
    window = init_opengl(windowWidth, windowHeight);
    if (window == nullptr) return -1;
    CLState* clState = init_opencl();

    // Initialize the Scene
    Scene* scene = new TokamakScene(state, torus, solenoid);
    scene->initialize();

    std::vector<CurrentVector> currents = scene->get_currents();

    // Create shared OpenCL buffers from OpenGL buffers
    cl_int posErr, velErr, eFieldVecErr, bFieldVecErr;
    state.particlePosBufCL = scene->getParticlePosBufCL(clState->context);
    state.particleVelBufCL = scene->getParticleVelBufCL(clState->context);
    state.eFieldVecBufCL   = scene->getEFieldVecBufCL(clState->context);
    state.bFieldVecBufCL   = scene->getBFieldVecBufCL(clState->context);
    state.eTracerBufCL     = scene->getETracerBufCL(clState->context);
    state.bTracerBufCL     = scene->getBTracerBufCL(clState->context);

    std::vector<cl::Memory> particlesKernelGLBuffers = {state.particlePosBufCL, state.particleVelBufCL};
    std::vector<cl::Memory> fieldsKernelGLBuffers = {state.particlePosBufCL, state.particleVelBufCL, state.eFieldVecBufCL, state.bFieldVecBufCL};
    std::vector<cl::Memory> tracerKernelGLBuffers = {state.eTracerBufCL, state.bTracerBufCL, state.particlePosBufCL, state.particleVelBufCL};
    std::vector<cl::Memory> defragKernelGLBuffers = {state.particlePosBufCL, state.particleVelBufCL};

    // Create additional OpenCL buffers
    std::vector<cl_float4> dbgBuf(state.maxParticles);
    std::vector<cl_float4> cellLocations;
    for (auto& cell : state.cells) {
        cellLocations.push_back(cell.pos);
    }
    state.nParticlesCL = cl::Buffer(*clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &state.initialParticles);
    cl::Buffer dbgBufCL(*clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * state.maxParticles, dbgBuf.data());
    cl::Buffer cellLocationBufCL(*clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * cellLocations.size(), cellLocations.data());
    cl::Buffer currentSegmentBufCL = get_current_segment_buffer(clState->context, currents);

    // Set up particle kernel parameters
    cl::Program particlesProgram = build_kernel(clState, "kernel/particles.cl", {"kernel/physical_constants.h", "kernel/field_common.cl"});
    cl::Kernel particlesKernel(particlesProgram, "computeMotion");
    particlesKernel.setArg(0, state.nParticlesCL);
    particlesKernel.setArg(1, state.particlePosBufCL);
    particlesKernel.setArg(2, state.particleVelBufCL);
    particlesKernel.setArg(3, currentSegmentBufCL);
    particlesKernel.setArg(4, dbgBufCL);
    particlesKernel.setArg(5, state.dt);
    particlesKernel.setArg(6, (cl_uint)currents.size());
    particlesKernel.setArg(7, state.solenoidFlux);
    particlesKernel.setArg(8, (cl_uint)state.enableParticleFieldContributions);

    // Set up field kernel parameters
    cl::Program fieldsProgram = build_kernel(clState, "kernel/fields.cl", {"kernel/physical_constants.h", "kernel/field_common.cl"});
    cl::Kernel fieldsKernel(fieldsProgram, "computeFields");
    fieldsKernel.setArg(0, state.nParticlesCL);
    fieldsKernel.setArg(1, cellLocationBufCL);
    fieldsKernel.setArg(2, state.eFieldVecBufCL);
    fieldsKernel.setArg(3, state.bFieldVecBufCL);
    fieldsKernel.setArg(4, state.particlePosBufCL);
    fieldsKernel.setArg(5, state.particleVelBufCL);
    fieldsKernel.setArg(6, currentSegmentBufCL);
    fieldsKernel.setArg(7, dbgBufCL);
    fieldsKernel.setArg(8, (cl_uint)state.cells.size());
    fieldsKernel.setArg(9, (cl_uint)currents.size());
    fieldsKernel.setArg(10, state.solenoidFlux);
    fieldsKernel.setArg(11, (cl_uint)state.enableParticleFieldContributions);

    // Set up defrag kernel parameters
    cl::Program defragProgram = build_kernel(clState, "kernel/defrag.cl", {"kernel/physical_constants.h"});
    cl::Kernel defragKernel(defragProgram, "defrag");
    defragKernel.setArg(0, state.nParticlesCL);
    defragKernel.setArg(1, state.particlePosBufCL);
    defragKernel.setArg(2, state.particleVelBufCL);
    defragKernel.setArg(3, state.maxParticles);

    // Set up tracer kernel parameters
    cl::Program eTracerProgram = build_kernel(clState, "kernel/e_tracer.cl", {"kernel/physical_constants.h", "kernel/field_common.cl"});
    cl::Kernel eTracerKernel(eTracerProgram, "updateTrails");
    eTracerKernel.setArg(0, state.nParticlesCL);
    eTracerKernel.setArg(1, state.eTracerBufCL);
    eTracerKernel.setArg(2, state.particlePosBufCL);
    eTracerKernel.setArg(3, state.particleVelBufCL);
    eTracerKernel.setArg(4, currentSegmentBufCL);
    eTracerKernel.setArg(5, (cl_uint)currents.size());
    eTracerKernel.setArg(6, state.solenoidFlux);
    eTracerKernel.setArg(7, (cl_uint)state.enableParticleFieldContributions);
    eTracerKernel.setArg(8, (cl_uint)scene->getNumTracers());
    eTracerKernel.setArg(9, (cl_uint)scene->getTracerPoints());

    cl::Program bTracerProgram = build_kernel(clState, "kernel/b_tracer.cl", {"kernel/physical_constants.h", "kernel/field_common.cl"});
    cl::Kernel bTracerKernel(bTracerProgram, "updateTrails");
    bTracerKernel.setArg(0, state.nParticlesCL);
    bTracerKernel.setArg(1, state.bTracerBufCL);
    bTracerKernel.setArg(2, state.particlePosBufCL);
    bTracerKernel.setArg(3, state.particleVelBufCL);
    bTracerKernel.setArg(4, currentSegmentBufCL);
    bTracerKernel.setArg(5, (cl_uint)currents.size());
    bTracerKernel.setArg(6, (cl_uint)state.enableParticleFieldContributions);
    bTracerKernel.setArg(7, (cl_uint)scene->getNumTracers());
    bTracerKernel.setArg(8, (cl_uint)scene->getTracerPoints());

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    cl_uint nParticles = state.initialParticles;
    int frameCount = 0;
    int simulationStep = 0;
    float frameTimeSec = 1.0f / (float)targetFPS;
    bool pEnableToroidalRings = state.enableToroidalRings;
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // Process keyboard input
        process_input(window, state, scene);

        // Draw a white background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene->render(aspectRatio);

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::chrono::duration<double> frameDur;
        do {
            // Update kernel args that could have changed
            state.toroidalI = state.enableToroidalRings ? torus.maxToroidalI : 0.0f;
            state.solenoidFlux = state.enableSolenoidFlux ? solenoid.maxSolenoidFlux : 0.0f;
            particlesKernel.setArg(5, state.dt);
            particlesKernel.setArg(7, state.solenoidFlux);
            particlesKernel.setArg(8, (cl_uint)state.enableParticleFieldContributions);
            fieldsKernel.setArg(10, state.solenoidFlux);
            fieldsKernel.setArg(11, (cl_uint)state.enableParticleFieldContributions);
            eTracerKernel.setArg(6, state.solenoidFlux);
            eTracerKernel.setArg(7, (cl_uint)state.enableParticleFieldContributions);
            bTracerKernel.setArg(6, (cl_uint)state.enableParticleFieldContributions);

            // Update current segment buffer if toroidal rings have been toggled
            if (pEnableToroidalRings != state.enableToroidalRings) {
                currents = scene->get_currents();
                update_currents_buffer(clState->queue, currentSegmentBufCL, currents);
                pEnableToroidalRings = state.enableToroidalRings;
            }

            // Compute fields
            // Acquire the GL buffer for OpenCL to read and write
            clState->queue->enqueueAcquireGLObjects(&fieldsKernelGLBuffers);
            cl_int fieldsKernelErr = clState->queue->enqueueNDRangeKernel(fieldsKernel, cl::NullRange, cl::NDRange(state.cells.size()));
            cl_exit_if_err(fieldsKernelErr, "Failed to enqueue kernel");
            clState->queue->enqueueReleaseGLObjects(&fieldsKernelGLBuffers);
            clState->queue->finish();

            // Update tracers
            // Acquire the GL buffer for OpenCL to read and write
            clState->queue->enqueueAcquireGLObjects(&tracerKernelGLBuffers);
            clState->queue->enqueueNDRangeKernel(eTracerKernel, cl::NullRange, cl::NDRange(scene->getNumTracers()));
            clState->queue->enqueueReleaseGLObjects(&tracerKernelGLBuffers);
            clState->queue->finish();
            // Acquire the GL buffer for OpenCL to read and write
            clState->queue->enqueueAcquireGLObjects(&tracerKernelGLBuffers);
            clState->queue->enqueueNDRangeKernel(bTracerKernel, cl::NullRange, cl::NDRange(scene->getNumTracers()));
            clState->queue->enqueueReleaseGLObjects(&tracerKernelGLBuffers);
            clState->queue->finish();

            // Do particle physics
            // Acquire the GL buffer for OpenCL to read and write
            clState->queue->enqueueAcquireGLObjects(&particlesKernelGLBuffers);
            cl_int particlesKernelErr = clState->queue->enqueueNDRangeKernel(particlesKernel, cl::NullRange, cl::NDRange(nParticles));
            cl_exit_if_err(particlesKernelErr, "Failed to enqueue kernel");
            if (simulationStep % 100 == 0) {
                std::cout << "SIM STEP " << simulationStep << " (frame " << frameCount << ") [" << nParticles << " particles]" << std::endl;
            }
            clState->queue->enqueueReleaseGLObjects(&particlesKernelGLBuffers);
            clState->queue->finish();

            // // Defrag particle buffers
            // // Acquire the GL buffer for OpenCL to read and write
            // clState->queue->enqueueAcquireGLObjects(&defragKernelGLBuffers);
            // cl_int defragKernelErr = clState->queue->enqueueNDRangeKernel(defragKernel, cl::NullRange, cl::NDRange(1));
            // cl_exit_if_err(defragKernelErr, "Failed to enqueue kernel");
            // clState->queue->enqueueReadBuffer(state.nParticlesCL, CL_TRUE, 0, sizeof(cl_uint), &nParticles);
            // clState->queue->enqueueReleaseGLObjects(&defragKernelGLBuffers);
            // clState->queue->finish();

            frameDur = std::chrono::high_resolution_clock::now() - frameStart;
            simulationStep++;
        } while (frameDur.count() < frameTimeSec);

        state.t += state.dt;
        frameCount++;
    }

    glfwTerminate();
    return 0;
}
