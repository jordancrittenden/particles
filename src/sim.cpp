#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/OpenGL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "cl_util.h"
#include "gl_util.h"
#include "keyboard.h"
#include "args.h"
#include "state.h"
#include "scene.h"
#include "torus.h"
#include "particles.h"
#include "axes.h"
#include "field_vector.h"

TorusProperties torus;
SimulationState state;
Scene scene;

void printDbg(const cl::Buffer& dbgBufCL, int n) {
    std::vector<cl_float4> dbgBuf(n);

    // Retrieve updated positions (optional, for debugging or visualization)
    state.clState->queue->enqueueReadBuffer(dbgBufCL, CL_TRUE, 0, sizeof(cl_float4) * n, dbgBuf.data());

    // Print out some particle positions for debugging
    for (int i = 0; i < 12; i++) { // Print only the first 5 entries for brevity
        std::cout << "Particle " << i << ": Debug (" 
                    << dbgBuf[i].s[0] << ", " 
                    << dbgBuf[i].s[1] << ", " 
                    << dbgBuf[i].s[2] << ", " 
                    << dbgBuf[i].s[3] << ")\n";
    }
}

// Main function
int main(int argc, char* argv[]) {
    // Parse CLI arguments into state variables
    try {
        auto args = parse_args(argc, argv);
        for (const auto& [key, value] : args) {
            std::cout << key << " : " << value << std::endl;
        }
        extract_state_vars(args, &state, &torus);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    state.window = init_opengl(state.windowWidth, state.windowHeight);
    if (state.window == nullptr) return -1;
    state.clState = init_opencl();

    // Load OpenGL shaders
    GLuint axesShaderProgram      = create_shader_program("shader/axes_vertex.glsl", "shader/axes_fragment.glsl");
    GLuint particlesShaderProgram = create_shader_program("shader/particles_vertex.glsl", "shader/particles_fragment.glsl");
    GLuint torusShaderProgram     = create_shader_program("shader/torus_vertex.glsl", "shader/torus_fragment.glsl");
    GLuint vectorShaderProgram    = create_shader_program("shader/vector_vertex.glsl", "shader/vector_fragment.glsl");

    // Initialize particles
    create_particle_buffers(torus, scene.pos, scene.vel, state.nParticles);

    // Create GL buffers for axes and particles
    scene.axes = create_axes_buffers();

    // Create GL buffers for torus
    scene.torus = create_torus_buffers(torus);

    std::vector<CurrentVector> torusCurrents = get_toroidal_currents(torus);

    std::vector<Cell> cells = get_torus_simulation_cells(torus, 60, 10, 20);
    std::vector<glm::vec4> eFieldLoc, eFieldVec;
    std::vector<glm::vec4> bFieldLoc, bFieldVec;
    for (auto& cell : cells) {
        eFieldLoc.push_back(glm::vec4(cell.pos, 0.0f)); // Last element indicates E vs B
        eFieldVec.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        bFieldLoc.push_back(glm::vec4(cell.pos, 1.0f)); // Last element indicates E vs B
        bFieldVec.push_back(glm::vec4(-1.0f, 1.0f, 0.0f, 0.0f));
    }

    scene.e_field = create_vectors_buffers(eFieldLoc, eFieldVec, 0.03f);
    scene.b_field = create_vectors_buffers(bFieldLoc, bFieldVec, 0.03f);

    // Create shared OpenCL buffers from the OpenGL buffer
    cl_int posErr, velErr, eFieldVecErr, bFieldVecErr;
    cl::Buffer particlePosBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.pos.vbo, &posErr);
    cl::Buffer particleVelBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.vel.vbo, &velErr);
    cl::Buffer eFieldVecBufCL   = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.e_field.instanceVecBuf, &eFieldVecErr);
    cl::Buffer bFieldVecBufCL   = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.b_field.instanceVecBuf, &bFieldVecErr);
    if (posErr != CL_SUCCESS || velErr != CL_SUCCESS || eFieldVecErr != CL_SUCCESS || bFieldVecErr != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer from OpenGL buffer: " << posErr << std::endl;
        return -1;
    }
    std::vector<cl::Memory> particleKernelGLBuffers = {particlePosBufCL, particleVelBufCL};
    std::vector<cl::Memory> fieldKernelGLBuffers = {particlePosBufCL, particleVelBufCL, eFieldVecBufCL, bFieldVecBufCL};

    // Create additional OpenCL buffers
    std::vector<cl_float4> dbgBuf(state.nParticles);
    std::vector<cl_float4> torusCurrentsUnrolled;
    for (auto& cur : torusCurrents) {
        torusCurrentsUnrolled.push_back(cl_float4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    std::vector<cl_float4> cellLocations;
    for (auto& cell : cells) {
        cellLocations.push_back(cl_float4 { cell.pos.x, cell.pos.y, cell.pos.z, 0.0f });
    }
    cl::Buffer dbgBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * state.nParticles, dbgBuf.data());
    cl::Buffer currentSegmentBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * torusCurrentsUnrolled.size(), torusCurrentsUnrolled.data());
    cl::Buffer cellLocationBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * cellLocations.size(), cellLocations.data());

    // Set up particle kernel parameters
    cl::Program particlesProgram = build_kernel(state.clState, "kernel/particles.cl");
    cl::Kernel particlesKernel(particlesProgram, "computeMotion");
    particlesKernel.setArg(0, particlePosBufCL);
    particlesKernel.setArg(1, particleVelBufCL);
    particlesKernel.setArg(2, currentSegmentBufCL);
    particlesKernel.setArg(3, dbgBufCL);
    particlesKernel.setArg(4, state.dt);
    particlesKernel.setArg(5, state.nParticles);
    particlesKernel.setArg(6, (cl_uint)torusCurrents.size());
    particlesKernel.setArg(7, /* solenoidFlux= */ 0.0f);
    particlesKernel.setArg(8, (cl_uint)state.calcInterparticlePhysics);

    // Set up field kernel parameters
    cl::Program fieldsProgram = build_kernel(state.clState, "kernel/fields.cl");
    cl::Kernel fieldsKernel(fieldsProgram, "computeFields");
    fieldsKernel.setArg(0, cellLocationBufCL);
    fieldsKernel.setArg(1, eFieldVecBufCL);
    fieldsKernel.setArg(2, bFieldVecBufCL);
    fieldsKernel.setArg(3, particlePosBufCL);
    fieldsKernel.setArg(4, particleVelBufCL);
    fieldsKernel.setArg(5, currentSegmentBufCL);
    fieldsKernel.setArg(6, dbgBufCL);
    fieldsKernel.setArg(7, (cl_uint)cells.size());
    fieldsKernel.setArg(8, state.nParticles);
    fieldsKernel.setArg(9, (cl_uint)torusCurrents.size());
    fieldsKernel.setArg(10, /* solenoidFlux= */ 0.0f);
    fieldsKernel.setArg(11, (cl_uint)state.calcInterparticlePhysics);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    while (!glfwWindowShouldClose(state.window)) {
        // Process keyboard input
        process_input(state.window, state, scene);

        // Update args that could have changed
        float solenoidFlux = state.pulseSolenoid ? torus.solenoidFlux : 0.0f;
        particlesKernel.setArg(4, state.dt);
        particlesKernel.setArg(7, solenoidFlux);
        fieldsKernel.setArg(10, solenoidFlux);

        // Compute fields
        // Acquire the GL buffer for OpenCL to read and write
        state.clState->queue->enqueueAcquireGLObjects(&fieldKernelGLBuffers);
        state.clState->queue->enqueueNDRangeKernel(fieldsKernel, cl::NullRange, cl::NDRange(cells.size()));
        //printDbg(dbgBufCL, cells.size());
        // Release the buffer back to OpenGL
        state.clState->queue->enqueueReleaseGLObjects(&fieldKernelGLBuffers);
        state.clState->queue->finish();

        // Do particle physics
        // Acquire the GL buffer for OpenCL to read and write
        state.clState->queue->enqueueAcquireGLObjects(&particleKernelGLBuffers);
        state.clState->queue->enqueueNDRangeKernel(particlesKernel, cl::NullRange, cl::NDRange(state.nParticles));
        //printDbg(particlePosBufCL, dbgBufCL);
        // Release the buffer back to OpenGL
        state.clState->queue->enqueueReleaseGLObjects(&particleKernelGLBuffers);
        state.clState->queue->finish();

        // Draw a white background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float cameraX = scene.cameraDistance * sin(scene.cameraTheta) * sin(scene.cameraPhi);
        float cameraY = scene.cameraDistance * cos(scene.cameraTheta);
        float cameraZ = scene.cameraDistance * sin(scene.cameraTheta) * cos(scene.cameraPhi);
        glm::mat4 view = glm::lookAt(
            glm::vec3(cameraX, cameraY, cameraZ), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)state.windowWidth / (float)state.windowHeight, 0.1f, 100.0f);

        if (scene.showAxes)      render_axes(axesShaderProgram, scene.axes, view, projection);
        if (scene.showTorus)     render_torus(torusShaderProgram, torus, scene.torus, view, projection);
        if (scene.showParticles) render_particles(particlesShaderProgram, scene.pos, state.nParticles, view, projection);
        if (scene.showEField)    render_fields(vectorShaderProgram, cells.size(), scene.e_field, view, projection);
        if (scene.showBField)    render_fields(vectorShaderProgram, cells.size(), scene.b_field, view, projection);

        glfwSwapBuffers(state.window);
        glfwPollEvents();

        state.t += state.dt;
    }

    glDeleteVertexArrays(1, &scene.axes.vao);
    glDeleteVertexArrays(1, &scene.pos.vao);
    glDeleteVertexArrays(1, &scene.torus.vao);
    glDeleteVertexArrays(1, &scene.e_field.arrowBuf.vao);
    glDeleteVertexArrays(1, &scene.b_field.arrowBuf.vao);

    glfwTerminate();
    return 0;
}
