#pragma once

#include <unordered_map>
#include <glm/glm.hpp>
#include "physical_constants.h"

enum SceneType {
    SCENE_TYPE_FREE_SPACE,
    SCENE_TYPE_TOKAMAK,
};

struct SimulationParams {
    SceneType sceneType = SCENE_TYPE_TOKAMAK;

    // Rendering parameters
    glm::u32 windowWidth = 1500;                 // Window width, px
    glm::u32 windowHeight = 1200;                // Window height, px
    glm::u16 targetFPS = 60;                     // Target FPS
    glm::f32 tracerDensity = 2.0;                // Tracer density, % of cells

    // Particle initialization parameters
    glm::f32 initialTemperature = 100000.0 * _K; // Initial plasma temperature, K
    glm::u32 initialParticles = 100000;          // Number of initial particles
    glm::u32 maxParticles = 150000;              // Maximum number of particles
    glm::f32 dt = 1e-11f * _S;                   // Simulation dt, s (keep electron w_ce*dt <~ 0.2; w_ce ~ 2e10 rad/s at 0.12 T)

    // Cell parameters
    glm::f32 cellSpacing = 0.05f * _M;           // Distance between simulation mesh cells, m
};

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]);

SimulationParams extract_params(std::unordered_map<std::string, std::string> args);