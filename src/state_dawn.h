#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "physical_constants.h"

typedef struct Cell {
    glm::f32vec4 pos; // [centerX, centerY, centerZ, isActive]
} Cell;

typedef struct SimulationState {
    // Particle initialization parameters
    glm::f32 initialTemperature = 300 * _K; // Initial plasma temperature, K
    glm::u32 initialParticles = 10000;      // Number of initial particles
    glm::u32 maxParticles = 40000;          // Maximum number of particles
    glm::f32 pctFreeElectrons = 0.01f;      // Percent of initial particles that are free electrons
    glm::f32 pctDeuterium = 0.495f;         // Percent of initial particles that are deuterium
    glm::f32 pctTritium = 0.495f;           // Percent of initial particles that are tritium

    // State variables
    std::vector<Cell> cells;                // Simulation cells
    // cl::BufferGL particlePosBufCL;          // Particle positions
    // cl::BufferGL particleVelBufCL;          // Particle velocities
    // cl::BufferGL eFieldVecBufCL;            // The E field at each cell center
    // cl::BufferGL bFieldVecBufCL;            // The B field at each cell center
    // cl::BufferGL eTracerBufCL;              // E field tracer trails
    // cl::BufferGL bTracerBufCL;              // B field tracer trails
    // cl::Buffer nParticlesCL;                // Current number of particles
    glm::f32 t = 0.0f * _S;                 // Simulation time, s
    glm::f32 dt = 1e-8f * _S;               // Simulation dt, s
    glm::f32 cellSpacing = 0.08f * _M;      // Distance between simulation grid cells, m
    glm::f32 solenoidFlux = 0.0f * _V * _S; // Flux through the solenoid, V s
    glm::f32 toroidalI = 50000.0f * _A;     // Current through the toroidal coils, A

    // Physics booleans
    bool enableParticleFieldContributions = false;
    bool enableToroidalRings = true;
    bool enableSolenoidFlux = false;
} SimulationState;

void print_state(const SimulationState& state);
glm::f32vec4 free_space_rand_particle_position(glm::f32vec3 minCoord, glm::f32vec3 maxCoord);
std::vector<Cell> get_free_space_grid_cells(glm::f32vec3 minCoord, glm::f32vec3 maxCoord, glm::f32 dx);