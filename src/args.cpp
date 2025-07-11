#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>

#include "args.h"

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

SceneType parse_scene_type(std::string sceneType) {
    if (sceneType == "tokamak") {
        return SCENE_TYPE_TOKAMAK;
    } else if (sceneType == "free_space") {
        return SCENE_TYPE_FREE_SPACE;
    } else {
        throw std::invalid_argument("Invalid scene type: " + sceneType);
    }
}

SimulationParams extract_params(std::unordered_map<std::string, std::string> args) {
    SimulationParams params;
     for (const auto& [key, value] : args) {
             if (key == "scene")              params.sceneType           = parse_scene_type(value);
        else if (key == "initialParticles")   params.initialParticles    = stoi(value);
        else if (key == "initialTemperature") params.initialTemperature  = stoi(value) * _K;
        else if (key == "maxParticles")       params.maxParticles        = stoi(value);
        else if (key == "dt")                 params.dt                  = stof(value) * _S;
        else if (key == "fps")                params.targetFPS           = stoi(value);
        else if (key == "width")              params.windowWidth         = stoi(value);
        else if (key == "height")             params.windowHeight        = stoi(value);
        else if (key == "cellSpacing")        params.cellSpacing         = stof(value) * _M;
        else throw std::invalid_argument("Invalid argument '" + key + "'");
     }
    return params;
}