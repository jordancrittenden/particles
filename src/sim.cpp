#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenCL/cl.hpp>
#include <OpenCL/cl_gl.h>
#include <OpenGL/OpenGL.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include "shader.h"


#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define N 1000 // number of particles
#define DELTA_TIME 0.0001f

GLFWwindow* window;

// Particle state buffers
GLuint posVAO, posVBO; // [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
GLuint velVAO, velVBO; // [x0, y0, z0, unused, x1, y1, z1, unused, ...]

// Camera settings
float angleX = 0.0f, angleY = 0.0f;
float cameraDistance = 5.0f;


// Create buffers for particle position/velocity
void create_particle_buffers() {
    std::vector<float> position_and_type;
    std::vector<float> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < N; ++i) {
        float chargeRand = static_cast<float>(rand()) / RAND_MAX;
        float charge = chargeRand < 0.33 ? 0.0 : (chargeRand < 0.66 ? 1.0 : -1.0);
        //float charge = i == 0 ? 1.0f : -1.0f;

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
    glGenVertexArrays(1, &posVAO);
    glBindVertexArray(posVAO);
    glGenBuffers(1, &posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(cl_float4), position_and_type.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)0); // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)(3 * sizeof(float))); // charge attribute
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // velocity buffer
    glGenVertexArrays(1, &velVAO);
    glBindVertexArray(velVAO);
    glGenBuffers(1, &velVBO);
    glBindBuffer(GL_ARRAY_BUFFER, velVBO);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(cl_float4), velocity.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Create buffer for axes (X, Y, Z)
GLuint create_axes_buffer() {
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

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// GLFW callback for handling keyboard input
void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        angleX -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        angleX += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        angleY -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        angleY += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        cameraDistance *= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        cameraDistance /= 1.01f;
    }
}

int init_opengl() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ParticleSim", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);

    return 0;
}

// Main function
int main() {
    int success = init_opengl();
    if (success != 0) {
        return -1;
    }

    // Load OpenGL shaders
    GLuint shaderProgram = createShaderProgram("shader/vertex_shader.glsl", "shader/fragment_shader.glsl");
    glUseProgram(shaderProgram);

    // Initialize particles
    create_particle_buffers();

    // Create VAOs for axes and particles
    GLuint axesVAO = create_axes_buffer();

    // Obtain the current OpenGL CGL sharegroup
    CGLContextObj cglContext = CGLGetCurrentContext();
    CGLShareGroupObj shareGroup = CGLGetShareGroup(cglContext);

    if (!shareGroup) {
        std::cerr << "Failed to get CGL share group" << std::endl;
        return -1;
    }

    // Initialize OpenCL
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) {
        std::cerr << "No OpenCL platforms found" << std::endl;
        return -1;
    }

    // Choose the first platform
    cl::Platform platform = platforms.front();
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if (devices.empty()) {
        std::cerr << "No GPU devices found for the chosen platform" << std::endl;
        return -1;
    }

    // Choose the first device
    cl::Device device = devices.front();
    std::cout << "Using OpenCL device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    // Check for OpenGL-OpenCL sharing support
    std::string extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();
    if (extensions.find("cl_APPLE_gl_sharing") == std::string::npos) {
        std::cerr << "OpenCL-OpenGL sharing is not supported on this device" << std::endl;
        return -1;
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
    cl::Context clContext(devices, properties, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL context with OpenGL sharing: " << err << std::endl;
        return -1;
    }

    // Create OpenCL command queue
    cl::CommandQueue queue(clContext, device, 0, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL command queue: " << err << std::endl;
        return -1;
    }

    std::cout << "OpenCL-OpenGL interop context created successfully." << std::endl;

    // Load kernel source
    std::ifstream kernelFile("kernel/particles.cl");
    std::string src(std::istreambuf_iterator<char>(kernelFile), (std::istreambuf_iterator<char>()));
    cl::Program program(clContext, src);
    cl_int buildErr = program.build(devices);
    
    if (buildErr != CL_SUCCESS) {
        // Retrieve and print the error log
        std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cerr << "Build failed with errors:\n" << buildLog << std::endl;
    } else {
        std::cout << "Program built successfully!" << std::endl;
    }

    // Create a shared OpenCL buffer from the OpenGL buffer
    cl_int posErr, velErr;
    std::vector<cl_float4> dbgBuf(N);
    cl::Buffer posBufCL = cl::BufferGL(clContext, CL_MEM_READ_WRITE, posVBO, &posErr);
    cl::Buffer velBufCL = cl::BufferGL(clContext, CL_MEM_READ_WRITE, velVBO, &velErr);
    cl::Buffer dbgBufCL(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float4) * N, dbgBuf.data());
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
    kernel.setArg(3, DELTA_TIME);
    kernel.setArg(4, N);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Main render loop
    std::vector<cl_float4> positions(N);
    int step = 0;
    while (!glfwWindowShouldClose(window)) {
        process_input(window);

        // Acquire the GL buffer for OpenCL to read and write
        queue.enqueueAcquireGLObjects(&glBuffers);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(N));

        // Retrieve updated positions (optional, for debugging or visualization)
        // queue.enqueueReadBuffer(posBufCL, CL_TRUE, 0, sizeof(cl_float4) * N, positions.data());
        // queue.enqueueReadBuffer(dbgBufCL, CL_TRUE, 0, sizeof(cl_float4) * N, dbgBuf.data());

        // // Print out some particle positions for debugging
        // for (int i = 0; i < N; i++) { // Print only the first 5 particles for brevity
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
        queue.enqueueReleaseGLObjects(&glBuffers);
        queue.finish();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Projection and view matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(cameraDistance * sin(angleY), 0.5f, cameraDistance * cos(angleY)), 
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw axes
        glBindVertexArray(axesVAO);
        glDrawArrays(GL_LINES, 0, 6);

        // Draw particles
        glBindVertexArray(posVAO);
        glDrawArrays(GL_POINTS, 0, N);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &axesVAO);
    glDeleteVertexArrays(1, &posVAO);

    glfwTerminate();
    return 0;
}
