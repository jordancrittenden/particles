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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        scene->rotateLeft();
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        scene->rotateRight();
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        scene->rotateDown();
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        scene->rotateUp();
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        scene->zoomOut();
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        scene->zoomIn();
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && debounce_input()) {
        state.dt *= 1.2f;
        std::cout << "dt: " << state.dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && debounce_input()) {
        state.dt /= 1.2f;
        std::cout << "dt: " << state.dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && debounce_input()) {
        scene->toggleShowAxes();
    }
    // if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && debounce_input()) {
    //     scene->showTorus = !scene->showTorus;
    // }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && debounce_input()) {
        scene->toggleShowEField();
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && debounce_input()) {
        scene->toggleShowBField();
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && debounce_input()) {
        scene->toggleShowParticles();
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && debounce_input()) {
        state.enableInterparticlePhysics = !state.enableInterparticlePhysics;
        std::cout << "interparticle physics: " << (state.enableInterparticlePhysics ? "ENABLED" : "DISABLED") << std::endl;
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