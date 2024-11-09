#pragma once

#include <unordered_map>
#include "state.h"
#include "torus.h"

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]);
void extract_state_vars(std::unordered_map<std::string, std::string> args, SimulationState* state, TorusProperties* torus);