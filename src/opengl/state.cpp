#include <iostream>
#include <glm/glm.hpp>
#include "state.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

void print_state(const SimulationState& state) {
    std::cout << "Simulation State:" << std::endl;
    std::cout << "  initial particles:" << state.initialParticles << std::endl;
    std::cout << "  max particles:" << state.maxParticles << std::endl;
    std::cout << "  particle field contributions: " << (state.enableParticleFieldContributions ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  toroidal rings: " << (state.enableToroidalRings ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  solenoid: " << (state.enableSolenoidFlux ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  dt:" << state.dt << std::endl;
}

cl_float4 free_space_rand_particle_position(glm::vec3 minCoord, glm::vec3 maxCoord) {
    float x = rand_range(minCoord.x, maxCoord.x);
    float y = rand_range(minCoord.y, maxCoord.y);
    float z = rand_range(minCoord.z, maxCoord.z);

    // [x, y, z, unused]
    return cl_float4 { x, y, z, 0.0f };
}

std::vector<Cell> get_free_space_grid_cells(glm::vec3 minCoord, glm::vec3 maxCoord, float dx) {
    std::vector<Cell> cells;
    for (float x = minCoord.x; x <= maxCoord.x; x += dx) {
        for (float z = minCoord.z; z <= maxCoord.z; z += dx) {
            for (float y = minCoord.y; y <= maxCoord.y; y += dx) {
                Cell cell;
                cell.pos = cl_float4 { x, y, z, 1.0f };
                cells.push_back(cell);
            }
        }
    }
    return cells;
}