#pragma once

#include "cl_util.h"

typedef struct SimulationState {
    // OpenCL state
    CLState* clState = nullptr;
    
    // Simulation parameters
    cl_uint nParticles = 10000; // Number of particles
    cl_float t = 0.0f;          // Simulation time, s
    cl_float dt = 0.0000001f;   // Simulation dt, s

    // Physics booleans
    bool enableInterparticlePhysics = false;
    bool enableToroidalRings = false;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);