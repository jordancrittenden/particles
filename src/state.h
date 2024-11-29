#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "cl_util.h"

typedef struct Cell {
    cl_float4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;

typedef struct SimulationState {
    // Particle initialization parameters
    cl_float initialTemperature = 300; // Initial plasma temperature, K
    cl_uint initialParticles = 10000;  // Number of initial particles
    cl_uint maxParticles = 40000;      // Maximum number of particles
    cl::Buffer nParticlesCL;           // Current number of particles
    cl_float pctFreeElectrons = 0.01f; // Percent of initial particles that are free electrons
    cl_float pctDeuterium = 0.495f;    // Percent of initial particles that are deuterium
    cl_float pctTritium = 0.495f;      // Percent of initial particles that are tritium

    // Simulation parameters
    cl_float t = 0.0f;                 // Simulation time, s
    cl_float dt = 1e-8f;               // Simulation dt, s
    cl_float cellSpacing = 0.05f;      // Distance between simulation grid cells, m

    // Physics booleans
    bool enableInterparticlePhysics = false;
    bool enableToroidalRings = false;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);
cl_float4 free_space_rand_particle_position(glm::vec3 minCoord, glm::vec3 maxCoord);
std::vector<Cell> get_free_space_grid_cells(glm::vec3 minCoord, glm::vec3 maxCoord, float dx);