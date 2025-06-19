#pragma once

#include <unordered_map>
#include <glm/glm.hpp>
#include "state_dawn.h"

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]);

void extract_state_vars(std::unordered_map<std::string, std::string> args, SimulationState* state, glm::u32* windowWidth, glm::u32* windowHeight, int* targetFPS);