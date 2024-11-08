#define GLM_ENABLE_EXPERIMENTAL

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
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
#include "field_vector.h"

#define PI   (3.14159265953f)
#define MU_0 (1.25663706144e-6f) /* kg m / A^2 s^2 */

#define NUM_VECTORS 100

TorusProperties torus;
SimulationState state;
Scene scene;

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

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

// Create buffers for axes (X, Y, Z)
GLBuffers create_axes_buffers() {
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

    GLBuffers buf;
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
        //float charge = chargeRand < 0.33 ? 0.0 : (chargeRand < 0.66 ? 1.0 : -1.0);
        float charge = chargeRand < 0.5 ? 1.0 : -1.0;

        float r = rand_range(0.95f, 1.05f);
        float theta = rand_range(0.0f, 2 * PI);
        float y = rand_range(-0.05f, 0.05f);

        // [x, y, z, type]
        position_and_type.push_back(r * sin(theta));
        position_and_type.push_back(y);
        position_and_type.push_back(r * cos(theta));
        position_and_type.push_back(charge);

        // [dx, dy, dz, unused]
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(0.0f);
    }

    // position/type buffer
    glGenVertexArrays(1, &scene.pos.vao);
    glBindVertexArray(scene.pos.vao);
    glGenBuffers(1, &scene.pos.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, scene.pos.vbo);
    glBufferData(GL_ARRAY_BUFFER, state.N * sizeof(cl_float4), position_and_type.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)0); // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)(3 * sizeof(float))); // charge attribute
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // velocity buffer
    glGenVertexArrays(1, &scene.vel.vao);
    glBindVertexArray(scene.vel.vao);
    glGenBuffers(1, &scene.vel.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, scene.vel.vbo);
    glBufferData(GL_ARRAY_BUFFER, state.N * sizeof(cl_float4), velocity.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLBuffers create_torus_buffers(float torusR2, int loopSegments) {
    GLBuffers buf;
    std::vector<float> circleVertices = generate_coil_vertices_unrolled(torusR2, loopSegments);

    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);
    glBindVertexArray(buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return buf;
}

void render_axes(GLuint shader, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw axes
    glBindVertexArray(scene.axes.vao);
    glDrawArrays(GL_LINES, 0, 6);
}

void render_particles(GLuint shader, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw particles
    glBindVertexArray(scene.pos.vao);
    glDrawArrays(GL_POINTS, 0, state.N);
}

void render_torus(GLuint shader, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(scene.torus.vao);

    // Draw each circle in the torus
    for (int i = 0; i < torus.toroidalCoils; ++i) {
        float angle = (2.0f * M_PI * i) / torus.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, torus.r1);

        GLint modelLoc = glGetUniformLocation(shader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_LINE_LOOP, 0, torus.coilLoopSegments);  // Draw the circle as a line loop
    }
}

void render_fields(GLuint shader, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(scene.e_field.vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, NUM_VECTORS);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 2, 5, NUM_VECTORS);
}

void printDbg(const cl::Buffer& posBufCL, const cl::Buffer& dbgBufCL) {
    std::vector<cl_float4> positions(state.N);
    std::vector<cl_float4> dbgBuf(state.N);

    // Retrieve updated positions (optional, for debugging or visualization)
    state.clState->queue->enqueueReadBuffer(posBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.N, positions.data());
    state.clState->queue->enqueueReadBuffer(dbgBufCL, CL_TRUE, 0, sizeof(cl_float4) * state.N, dbgBuf.data());

    // Print out some particle positions for debugging
    for (int i = 0; i < state.N; i++) { // Print only the first 5 particles for brevity
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
    GLuint torusShaderProgram = create_shader_program("shader/torus_vertex.glsl", "shader/torus_fragment.glsl");
    GLuint vectorShaderProgram = create_shader_program("shader/vector_vertex.glsl", "shader/vector_fragment.glsl");

    // Initialize particles
    create_particle_buffers();

    // Create GL buffers for axes and particles
    scene.axes = create_axes_buffers();

    // Create GL buffers for torus
    scene.torus = create_torus_buffers(torus.r2, torus.coilLoopSegments);

    std::vector<CurrentVector> torusCurrents = get_toroidal_currents(torus);

    std::vector<glm::vec3> directions = random_vectors(NUM_VECTORS);
    scene.e_field = create_vectors_buffers(directions, 0.5f);

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
    cl::Buffer posBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.pos.vbo, &posErr);
    cl::Buffer velBufCL = cl::BufferGL(*state.clState->context, CL_MEM_READ_WRITE, scene.vel.vbo, &velErr);
    if (posErr != CL_SUCCESS || velErr != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer from OpenGL buffer: " << posErr << std::endl;
        return -1;
    }
    std::vector<cl::Memory> glBuffers = {posBufCL, velBufCL};

    // Create additional OpenCL buffers
    std::vector<cl_float4> dbgBuf(state.N);
    std::vector<cl_float4> torusCurrentsUnrolled;
    for (auto& cur : torusCurrents) {
        torusCurrentsUnrolled.push_back(cl_float4 { cur.x[0], cur.x[1], cur.x[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.dx[0], cur.dx[1], cur.dx[2], 0.0f });
        torusCurrentsUnrolled.push_back(cl_float4 { cur.i, 0.0f, 0.0f, 0.0f });
    }
    cl::Buffer dbgBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * state.N, dbgBuf.data());
    cl::Buffer curBufCL(*state.clState->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * torusCurrentsUnrolled.size(), torusCurrentsUnrolled.data());

    // Set up kernel
    cl::Kernel kernel(program, "computeMotion");
    kernel.setArg(0, posBufCL);
    kernel.setArg(1, velBufCL);
    kernel.setArg(2, curBufCL);
    kernel.setArg(3, dbgBufCL);
    kernel.setArg(4, state.dt);
    kernel.setArg(5, state.N);
    kernel.setArg(6, (cl_uint)torusCurrents.size());
    kernel.setArg(7, 0.0f);
    kernel.setArg(8, (cl_uint)state.calcInterparticlePhysics);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    float pulseStartT = 0.0f;
    float solenoidK = 0.5f * torus.pulseAlpha * MU_0 * (float)torus.solenoidN * torus.solenoidI * (torus.solenoidR * torus.solenoidR);
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
            solenoidE0 = solenoidK * exp(-torus.pulseAlpha * pulseT);
            std::cout << "Pulse T: " << pulseT << ", E0: " << solenoidE0 << std::endl;
        }
        if (pulseT > torus.pulseAlpha) {
            pulseStartT = 0.0f;
        }

        // Update args that could have changed
        kernel.setArg(4, state.dt);
        kernel.setArg(7, solenoidE0);

        // Do particle physics
        // Acquire the GL buffer for OpenCL to read and write
        state.clState->queue->enqueueAcquireGLObjects(&glBuffers);
        state.clState->queue->enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(state.N));
        //printDbg(posBufCL, dbgBufCL);
        // Release the buffer back to OpenGL
        state.clState->queue->enqueueReleaseGLObjects(&glBuffers);
        state.clState->queue->finish();

        for (int i = 0; i < directions.size(); i++) {
            directions[i] = glm::rotateY(directions[i], glm::radians(0.1f));
        }
        update_vectors_buffer(scene.e_field.instance_vbo, directions);

        // Draw a white background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            glm::vec3(scene.cameraDistance * sin(scene.rotAngle), 0.5f, scene.cameraDistance * cos(scene.rotAngle)), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)state.windowWidth / (float)state.windowHeight, 0.1f, 100.0f);

        if (scene.showAxes) render_axes(particlesShaderProgram, view, projection);
        if (scene.showTorus) render_torus(torusShaderProgram, view, projection);
        if (scene.showParticles) render_particles(particlesShaderProgram, view, projection);
        if (scene.showEField) render_fields(vectorShaderProgram, view, projection);

        glfwSwapBuffers(state.window);
        glfwPollEvents();

        state.t += state.dt;
    }

    glDeleteVertexArrays(1, &scene.axes.vao);
    glDeleteVertexArrays(1, &scene.pos.vao);

    glfwTerminate();
    return 0;
}
