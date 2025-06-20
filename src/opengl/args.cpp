#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>

#include "state.h"
#include "scene.h"

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // Check if the argument starts with "--"
        if (arg.find("--") != 0) {
            throw std::invalid_argument("Invalid argument: " + arg + ". Arguments must start with '--'.");
        }

        // Find the position of '=' to separate the key and value
        size_t equalPos = arg.find('=');
        if (equalPos == std::string::npos) {
            throw std::invalid_argument("Invalid argument format: " + arg + ". Expected format is --key=value.");
        }

        // Extract key and value
        std::string key = arg.substr(2, equalPos - 2);  // Skip the "--" prefix
        std::string value = arg.substr(equalPos + 1);

        // Insert into the map
        args[key] = value;
    }

    return args;
}

void extract_state_vars(std::unordered_map<std::string, std::string> args, SimulationState* state, int* windowWidth, int* windowHeight, int* targetFPS) {
     for (const auto& [key, value] : args) {
             if (key == "initialParticles")           state->initialParticles                 = stoi(value);
        else if (key == "initialTemperature")         state->initialTemperature               = stoi(value) * _K;
        else if (key == "maxParticles")               state->maxParticles                     = stoi(value);
        else if (key == "fps")                        *targetFPS                              = stoi(value);
        else if (key == "width")                      *windowWidth                            = stoi(value);
        else if (key == "height")                     *windowHeight                           = stoi(value);
        else if (key == "cellSpacing")                state->cellSpacing                      = stof(value) * _M;
        else if (key == "particleFieldContributions") state->enableParticleFieldContributions = (value == "true");
        else throw std::invalid_argument("Invalid argument '" + key + "'");
     }
}