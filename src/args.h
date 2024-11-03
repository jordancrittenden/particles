#ifndef __ARGS_H__
#define __ARGS_H__

#include <unordered_map>
#include "state.h"

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]);
void extract_state_vars(std::unordered_map<std::string, std::string> args, SimulationState* state);
void print_state_vars(SimulationState* state);

#endif