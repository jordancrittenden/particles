#ifndef __STATE_H__
#define __STATE_H__

#include "cl_util.h"
#include "gl_util.h"

typedef struct SimulationState {
    // OpenGL window
    GLFWwindow* window = nullptr;
    int windowWidth = 1200;
    int windowHeight = 800;

    // OpenCL state
    CLState* clState = nullptr;
    
    // Simulation parameters
    cl_uint N = 1000; // number of particles
    cl_float dt = 0.0001f;

    // Torus geometry and buffers
    float torusR1 = 1.0f;        // Radius of torus
    float torusR2 = 0.2f;        // Radius of torus cross section
    int torusLoops = 10;         // Number of current loops
    int torusLoopSegments = 10; // Number of current segments per circle for approximation
    GLBufPair torus;

    // Particle state buffers
    GLBufPair pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBufPair vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBufPair axes;

    // Camera settings
    float rotAngle = 0.0f;
    float cameraDistance = 5.0f;
} SimulationState;

void print_state(const SimulationState& state);

#endif