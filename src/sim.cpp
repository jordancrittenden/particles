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
#include <ctime>

#include "cl_util.h"
#include "gl_util.h"
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

// GLFW callback for handling keyboard input
void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        scene.rotAngle -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        scene.rotAngle += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        scene.cameraDistance *= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        scene.cameraDistance /= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        state.dt *= 1.2f;
        print_state(state);
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        state.dt /= 1.2f;
        print_state(state);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        scene.showAxes = !scene.showAxes;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        scene.showTorus = !scene.showTorus;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        scene.showEField = !scene.showEField;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        scene.showParticles = !scene.showParticles;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        state.calcInterparticlePhysics = !state.calcInterparticlePhysics;
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        state.startPulse = true;
    }
}

void printDbg(const cl::Buffer& particlePosBufCL, const cl::Buffer& dbgBufCL) {
    std::vector<cl_float4> positions(state.nParticles);
    std::vector<cl_float4> dbgBuf(state.nParticles);

    // Retrieve updated positions (optional, for debugging or visualization)
    state.clState->queue->enqueueReadBuffer(particlePosBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.nParticles, positions.data());
    state.clState->queue->enqueueReadBuffer(dbgBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.nParticles, dbgBuf.data());

    // Print out some particle positions for debugging
    for (int i = 0; i < state.nParticles; i++) { // Print only the first 5 particles for brevity
        std::cout << "Particle " << i << ": Position (" 
                    << positions[i].s[0] << ", " 
                    << positions[i].s[1] << ", " 
                    << positions[i].s[2] << "), Debug (" 
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
    std::vector<glm::mat4> eFieldTranslation, eFieldRotation;
    for (auto& cell : cells) {
        glm::mat4 xform = glm::mat4(1.0f);
        eFieldTranslation.push_back(glm::translate(xform, cell.pos));
        eFieldRotation.push_back(glm::rotate(xform, glm::radians(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    scene.e_field = create_vectors_buffers(eFieldTranslation, eFieldRotation, 0.02f);

    // Create shared OpenCL buffers from the OpenGL buffer
    cl_int posErr, velErr;
    cl::Buffer particlePosBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.pos.vbo, &posErr);
    cl::Buffer particleVelBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.vel.vbo, &velErr);
    if (posErr != CL_SUCCESS || velErr != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer from OpenGL buffer: " << posErr << std::endl;
        return -1;
    }
    std::vector<cl::Memory> particleKernelGLBuffers = {particlePosBufCL, particleVelBufCL};

    // Create additional OpenCL buffers
    std::vector<cl_float4> dbgBuf(state.nParticles);
    std::vector<cl_float4> torusCurrentsUnrolled;
    for (auto& cur : torusCurrents) {
        torusCurrentsUnrolled.push_back(cl_float4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    cl::Buffer dbgBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * state.nParticles, dbgBuf.data());
    cl::Buffer currentSegmentBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * torusCurrentsUnrolled.size(), torusCurrentsUnrolled.data());

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
    particlesKernel.setArg(7, 0.0f);
    particlesKernel.setArg(8, (cl_uint)state.calcInterparticlePhysics);

    // Set up field kernel parameters
    cl::Program fieldsProgram = build_kernel(state.clState, "kernel/fields.cl");
    cl::Kernel fieldsKernel(fieldsProgram, "computeFields");
    // fieldsKernel.setArg(0, cellLocationBufCL);
    // fieldsKernel.setArg(1, eFieldBufCL);
    // fieldsKernel.setArg(2, bFieldBufCL);
    fieldsKernel.setArg(3, particlePosBufCL);
    fieldsKernel.setArg(4, particleVelBufCL);
    fieldsKernel.setArg(5, currentSegmentBufCL);
    fieldsKernel.setArg(6, (cl_uint)cells.size());
    fieldsKernel.setArg(7, state.nParticles);
    fieldsKernel.setArg(8, (cl_uint)torusCurrents.size());
    fieldsKernel.setArg(9, 0.0f);
    fieldsKernel.setArg(10, (cl_uint)state.calcInterparticlePhysics);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    float pulseStartT = 0.0f;
    while (!glfwWindowShouldClose(state.window)) {
        // Process keyboard input
        process_input(state.window);

        // Kick off a pulse of the central solenoid
        if (state.startPulse) {
            state.startPulse = false;
            pulseStartT = state.t;
        }
        float pulseT = state.t - pulseStartT;
        float solenoidE0 = 0.0f;
        if (pulseStartT > 0.0f) {
            solenoidE0 = solenoid_pulse_e_field_multiplier(torus, pulseT);
            std::cout << "Pulse T: " << pulseT << ", E0: " << solenoidE0 << std::endl;
        }
        if (pulseT > torus.pulseAlpha) {
            pulseStartT = 0.0f;
        }

        // Update args that could have changed
        particlesKernel.setArg(4, state.dt);
        particlesKernel.setArg(7, solenoidE0);

        // Do particle physics
        // Acquire the GL buffer for OpenCL to read and write
        state.clState->queue->enqueueAcquireGLObjects(&particleKernelGLBuffers);
        state.clState->queue->enqueueNDRangeKernel(particlesKernel, cl::NullRange, cl::NDRange(state.nParticles));
        //printDbg(particlePosBufCL, dbgBufCL);
        // Release the buffer back to OpenGL
        state.clState->queue->enqueueReleaseGLObjects(&particleKernelGLBuffers);
        state.clState->queue->finish();

        // Update field vectors
        for (int i = 0; i < eFieldRotation.size(); i++) {
            eFieldRotation[i] = glm::rotate(eFieldRotation[i], glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        update_vectors_buffer(scene.e_field, eFieldRotation);

        // Draw a white background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            glm::vec3(scene.cameraDistance * sin(scene.rotAngle), 0.5f, scene.cameraDistance * cos(scene.rotAngle)), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)state.windowWidth / (float)state.windowHeight, 0.1f, 100.0f);

        if (scene.showAxes)      render_axes(particlesShaderProgram, scene.axes, view, projection);
        if (scene.showTorus)     render_torus(torusShaderProgram, torus, scene.torus, view, projection);
        if (scene.showParticles) render_particles(particlesShaderProgram, scene.pos, state.nParticles, view, projection);
        if (scene.showEField)    render_fields(vectorShaderProgram, cells.size(), scene.e_field, view, projection);

        glfwSwapBuffers(state.window);
        glfwPollEvents();

        state.t += state.dt;
    }

    glDeleteVertexArrays(1, &scene.axes.vao);
    glDeleteVertexArrays(1, &scene.pos.vao);
    glDeleteVertexArrays(1, &scene.torus.vao);
    glDeleteVertexArrays(1, &scene.e_field.vectorBuf.vao);

    glfwTerminate();
    return 0;
}
