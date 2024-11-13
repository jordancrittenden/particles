#pragma once

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
    cl_uint nParticles = 10000; // Number of particles
    cl_float t = 0.0f;          // Simulation time, s
    cl_float dt = 0.0000001f;   // Simulation dt, s

    // Physics booleans
    bool calcInterparticlePhysics = true;
    bool pulseSolenoid = false; // If true, central solenoid is active
} SimulationState;

void print_state(const SimulationState& state);