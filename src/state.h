#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "physical_constants.h"
#include "cl_util.h"

typedef struct Cell {
    cl_float4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;

typedef struct SimulationState {
    // Particle initialization parameters
    cl_float initialTemperature = 300 * _K; // Initial plasma temperature, K
    cl_uint initialParticles = 10000;       // Number of initial particles
    cl_uint maxParticles = 40000;           // Maximum number of particles
    cl_float pctFreeElectrons = 0.01f;      // Percent of initial particles that are free electrons
    cl_float pctDeuterium = 0.495f;         // Percent of initial particles that are deuterium
    cl_float pctTritium = 0.495f;           // Percent of initial particles that are tritium

    // State variables
    std::vector<Cell> cells;                // Simulation cells
    cl::BufferGL particlePosBufCL;          // Particle positions
    cl::BufferGL particleVelBufCL;          // Particle velocities
    cl::BufferGL eFieldVecBufCL;            // The E field at each cell center
    cl::BufferGL bFieldVecBufCL;            // The B field at each cell center
    cl::BufferGL tracerBufCL;               // Tracer trails
    cl::Buffer nParticlesCL;                // Current number of particles
    cl_float t = 0.0f * _S;                 // Simulation time, s
    cl_float dt = 1e-8f * _S;               // Simulation dt, s
    cl_float cellSpacing = 0.08f * _M;      // Distance between simulation grid cells, m
    cl_float solenoidFlux = 0.0f * _V * _S; // Flux through the solenoid, V s
    cl_float toroidalI = 0.0f * _A;         // Current through the toroidal coils, A

    // Physics booleans
    bool enableParticleFieldContributions = false;
    bool enableToroidalRings = true;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);
cl_float4 free_space_rand_particle_position(glm::vec3 minCoord, glm::vec3 maxCoord);
std::vector<Cell> get_free_space_grid_cells(glm::vec3 minCoord, glm::vec3 maxCoord, float dx);