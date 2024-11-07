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
    cl_uint N = 10000;       // Number of particles
    cl_float t = 0.0f;       // Simulation time, s
    cl_float dt = 0.000001f; // Simulation dt, s

    // Physics booleans
    bool calcInterparticlePhysics = true;
    bool startPulse = true; // Kicks of a pulse of the central Solenoid
} SimulationState;

void print_state(const SimulationState& state);

#endif