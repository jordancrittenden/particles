#include <chrono>
#include <iostream>

#include "state.h"
#include "scene.h"

auto lastInputTime = std::chrono::high_resolution_clock::now();

bool debounce_input() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - lastInputTime;
    if (diff.count() < 0.1) return false;
    lastInputTime = now;
    return true;
}

// GLFW callback for handling keyboard input
void process_input(GLFWwindow* window, SimulationState& state, Scene* scene) {
    if (scene->process_input(window, debounce_input)) return;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && debounce_input()) {
        state.dt *= 1.2f;
        std::cout << "dt: " << state.dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && debounce_input()) {
        state.dt /= 1.2f;
        std::cout << "dt: " << state.dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && debounce_input()) {
        state.enableParticleFieldContributions = !state.enableParticleFieldContributions;
        std::cout << "particle field contributions: " << (state.enableParticleFieldContributions ? "ENABLED" : "DISABLED") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && debounce_input()) {
        state.enableToroidalRings = !state.enableToroidalRings;
        std::cout << "toroidal rings: " << (state.enableToroidalRings ? "ENABLED" : "DISABLED") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS && debounce_input()) {
        state.enableSolenoidFlux = !state.enableSolenoidFlux;
        std::cout << "solenoid: " << (state.enableSolenoidFlux ? "ENABLED" : "DISABLED") << std::endl;
    }
}