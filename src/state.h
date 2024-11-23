#pragma once

#include "cl_util.h"

typedef struct Cell {
    cl_float4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;

typedef struct SimulationState {
    // OpenCL state
    CLState* clState = nullptr;
    
    // Particle initialization parameters
    cl_float initialTemperature = 300; // Initial plasma temperature, K
    cl_uint nInitialParticles = 10000; // Number of initial particles
    cl_float pctFreeElectrons = 0.01f; // Percent of initial particles that are free electrons
    cl_float pctDeuterium = 0.495f;    // Percent of initial particles that are deuterium
    cl_float pctTritium = 0.495f;      // Percent of initial particles that are tritium

    // Simulation parameters
    cl_float t = 0.0f;                 // Simulation time, s
    cl_float dt = 0.0000001f;          // Simulation dt, s
    cl_float cellSpacing = 0.05f;      // Distance between simulation grid cells, m

    // Physics booleans
    bool enableInterparticlePhysics = false;
    bool enableToroidalRings = false;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);