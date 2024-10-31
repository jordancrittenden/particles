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
#include "state.h"

SimulationState state;

// Create buffer for axes (X, Y, Z)
GLBufPair create_axes_buffers() {
    float axisVertices[] = {
        // X-axis
        -1.0, 0.0f, 0.0f,   2.0f,
         1.0, 0.0f, 0.0f,   2.0f,
        // Y-axis
        0.0f, -1.0, 0.0f,   2.0f,
        0.0f,  1.0, 0.0f,   2.0f,
        // Z-axis
        0.0f, 0.0f, -1.0,   2.0f,
        0.0f, 0.0f,  1.0,   2.0f,
    };

    GLBufPair buf;
    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);

    glBindVertexArray(buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return buf;
}

// Create buffers for particle position/velocity
void create_particle_buffers() {
    std::vector<float> position_and_type;
    std::vector<float> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < state.N; ++i) {
        float chargeRand = static_cast<float>(rand()) / RAND_MAX;
        float charge = chargeRand < 0.33 ? 0.0 : (chargeRand < 0.66 ? 1.0 : -1.0);

        // [x, y, z, type]
        position_and_type.push_back(static_cast<float>(rand()) / RAND_MAX * 1.0f - 0.5f);
        position_and_type.push_back(static_cast<float>(rand()) / RAND_MAX * 1.0f - 0.5f);
        position_and_type.push_back(static_cast<float>(rand()) / RAND_MAX * 1.0f - 0.5f);
        position_and_type.push_back(charge);

        // [x, y, z, unused]
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 0.2f - 0.1f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 0.2f - 0.1f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 0.2f - 0.1f);
        velocity.push_back(0.0f);
    }

    // position/type buffer
    glGenVertexArrays(1, &state.pos.vao);
    glBindVertexArray(state.pos.vao);
    glGenBuffers(1, &state.pos.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state.pos.vbo);
    glBufferData(GL_ARRAY_BUFFER, state.N * sizeof(cl_float4), position_and_type.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)0); // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)(3 * sizeof(float))); // charge attribute
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // velocity buffer
    glGenVertexArrays(1, &state.vel.vao);
    glBindVertexArray(state.vel.vao);
    glGenBuffers(1, &state.vel.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, state.vel.vbo);
    glBufferData(GL_ARRAY_BUFFER, state.N * sizeof(cl_float4), velocity.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// GLFW callback for handling keyboard input
void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        state.rotAngle -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        state.rotAngle += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        state.cameraDistance *= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        state.cameraDistance /= 1.01f;
    }
}

// Main function
int main() {
    state.window = init_opengl(state.windowWidth, state.windowHeight);
    if (state.window == nullptr) return -1;
    state.clState = init_opencl();

    // Load OpenGL shaders
    GLuint shaderProgram = create_shader_program("shader/vertex_shader.glsl", "shader/fragment_shader.glsl");
    glUseProgram(shaderProgram);

    // Initialize particles
    create_particle_buffers();

    // Create VAOs for axes and particles
    state.axes = create_axes_buffers();

    // Load kernel source
    std::ifstream kernelFile("kernel/particles.cl");
    std::string src(std::istreambuf_iterator<char>(kernelFile), (std::istreambuf_iterator<char>()));
    cl::Program program(*state.clState->context, src);
    cl_int buildErr = program.build(state.clState->devices);
    
    if (buildErr != CL_SUCCESS) {
        // Retrieve and print the error log
        std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*state.clState->selectedDevice);
        std::cerr << "Build failed with errors:\n" << buildLog << std::endl;
    } else {
        std::cout << "Program built successfully!" << std::endl;
    }

    // Create a shared OpenCL buffer from the OpenGL buffer
    cl_int posErr, velErr;
    std::vector<cl_float4> dbgBuf(state.N);
    cl::Buffer posBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, state.pos.vbo, &posErr);
    cl::Buffer velBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, state.vel.vbo, &velErr);
    cl::Buffer dbgBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * state.N, dbgBuf.data());
    if (posErr != CL_SUCCESS || velErr != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer from OpenGL buffer: " << posErr << std::endl;
        return -1;
    }
    std::vector<cl::Memory> glBuffers = {posBufCL, velBufCL};

    // Set up kernel
    cl::Kernel kernel(program, "computeMotion");
    kernel.setArg(0, posBufCL);
    kernel.setArg(1, velBufCL);
    kernel.setArg(2, dbgBufCL);
    kernel.setArg(3, state.dt);
    kernel.setArg(4, state.N);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    std::vector<cl_float4> positions(state.N);
    int step = 0;
    while (!glfwWindowShouldClose(state.window)) {
        process_input(state.window);

        // Acquire the GL buffer for OpenCL to read and write
        state.clState->queue->enqueueAcquireGLObjects(&glBuffers);
        state.clState->queue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(state.N));

        // // Retrieve updated positions (optional, for debugging or visualization)
        // state.clState->queue->enqueueReadBuffer(posBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.state.N, positions.data());
        // state.clState->queue->enqueueReadBuffer(dbgBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.state.N, dbgBuf.data());

        // // Print out some particle positions for debugging
        // for (int i = 0; i < state.N; i++) { // Print only the first 5 particles for brevity
        //     std::cout << "Particle " << i << ": Position (" 
        //                 << positions[i].s[0] << ", " 
        //                 << positions[i].s[1] << ", " 
        //                 << positions[i].s[2] << "), Debug (" 
        //                 << dbgBuf[i].s[0] << ", " 
        //                 << dbgBuf[i].s[1] << ", " 
        //                 << dbgBuf[i].s[2] << ", " 
        //                 << dbgBuf[i].s[3] << ")\n";
        // }
        // step++;

        // Release the buffer back to OpenGL
        state.clState->queue->enqueueReleaseGLObjects(&glBuffers);
        state.clState->queue->finish();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Projection and view matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(state.cameraDistance * sin(state.rotAngle), 0.5f, state.cameraDistance * cos(state.rotAngle)), 
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)state.windowWidth / (float)state.windowHeight, 0.1f, 100.0f);

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw axes
        glBindVertexArray(state.axes.vao);
        glDrawArrays(GL_LINES, 0, 6);

        // Draw particles
        glBindVertexArray(state.pos.vao);
        glDrawArrays(GL_POINTS, 0, state.N);

        glfwSwapBuffers(state.window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &state.axes.vao);
    glDeleteVertexArrays(1, &state.pos.vao);

    glfwTerminate();
    return 0;
}
