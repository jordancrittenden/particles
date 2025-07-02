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
    glm::u32 windowWidth = 1024;            // Window width, px
    glm::u32 windowHeight = 768;            // Window height, px
    glm::u16 targetFPS = 60;                // Target FPS

    // Particle initialization parameters
    glm::f32 initialTemperature = 300 * _K; // Initial plasma temperature, K
    glm::u32 initialParticles = 100000;     // Number of initial particles
    glm::u32 maxParticles = 150000;         // Maximum number of particles

    // Cell parameters
    glm::f32 cellSpacing = 0.08f * _M;      // Distance between simulation grid cells, m
};

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]);

SimulationParams extract_params(std::unordered_map<std::string, std::string> args);