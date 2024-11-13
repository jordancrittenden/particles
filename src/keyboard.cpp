#include <chrono>

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
void process_input(GLFWwindow* window, SimulationState& state, Scene& scene) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        scene.cameraPhi -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        scene.cameraPhi += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        scene.cameraTheta += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        scene.cameraTheta -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        scene.cameraDistance *= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        scene.cameraDistance /= 1.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        state.dt *= 1.2f;
        print_state(state);
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        state.dt /= 1.2f;
        print_state(state);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && debounce_input()) {
        scene.showAxes = !scene.showAxes;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && debounce_input()) {
        scene.showTorus = !scene.showTorus;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && debounce_input()) {
        scene.showEField = !scene.showEField;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && debounce_input()) {
        scene.showBField = !scene.showBField;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && debounce_input()) {
        scene.showParticles = !scene.showParticles;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && debounce_input()) {
        state.calcInterparticlePhysics = !state.calcInterparticlePhysics;
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS && debounce_input()) {
        state.pulseSolenoid = !state.pulseSolenoid;
    }
}