#pragma once

#include "cl_util.h"

typedef struct Cell {
    cl_float4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;

typedef struct SimulationState {
    // OpenCL state
    CLState* clState = nullptr;
    
    // Simulation parameters
    cl_uint nParticles = 10000;   // Number of particles
    cl_float t = 0.0f;            // Simulation time, s
    cl_float dt = 0.0000001f;     // Simulation dt, s
    cl_float cellSpacing = 0.05f; // Distance between simulation grid cells, m

    // Physics booleans
    bool enableInterparticlePhysics = false;
    bool enableToroidalRings = false;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);