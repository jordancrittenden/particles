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
    cl_uint N = 10000; // number of particles
    cl_float dt = 0.00001f;

    // Physics booleans
    bool calcInterparticlePhysics = true;
} SimulationState;

void print_state(const SimulationState& state);

#endif