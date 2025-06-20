#include <glm/gtc/matrix_transform.hpp>
#include "tokamak_dawn.h"
#include "render/torus_dawn.h"
#include "render/solenoid_dawn.h"
#include "render/ring_dawn.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

TokamakScene::TokamakScene(SimulationState& state, TorusParameters& params, SolenoidParameters& solenoidParams) 
    : Scene(state), torusParameters(params), solenoidParameters(solenoidParams) {
}

void TokamakScene::initialize(wgpu::Device& device) {
    Scene::initialize(device);

    // Create torus buffers
    Ring toroidalRing;
    toroidalRing.r = torusParameters.r2;
    toroidalRing.t = 0.05f * _M;
    toroidalRing.d = 0.1f * _M;
    this->torusBuf = create_torus_buffers(device, toroidalRing, torusParameters.toroidalCoils);

    // Create solenoid buffers
    Ring solenoidRing;
    solenoidRing.r = solenoidParameters.r;
    solenoidRing.t = 0.05f * _M;
    solenoidRing.d = torusParameters.r2 * 2.0f;
    this->solenoidBuf = create_solenoid_buffers(device, solenoidRing);

    this->cameraDistance = 5.0f * _M;
}

TokamakScene::~TokamakScene() {
}

void TokamakScene::render(wgpu::Device& device, wgpu::RenderPassEncoder& pass, float aspectRatio) {
    Scene::render(device, pass, aspectRatio);
    if (this->showTorus) render_torus(device, pass, torusBuf, torusParameters.r1, view, projection);
    if (this->showSolenoid) render_solenoid(device, pass, solenoidBuf, view, projection);
}

std::vector<Cell> TokamakScene::get_grid_cells(float dx) {
    std::vector<Cell> cells;

    glm::vec3 minCoord(-(torusParameters.r1 + torusParameters.r2), -torusParameters.r2, -(torusParameters.r1 + torusParameters.r2));
    glm::vec3 maxCoord(torusParameters.r1 + torusParameters.r2, torusParameters.r2, torusParameters.r1 + torusParameters.r2);

    glm::f32vec4 torusCenterPos = glm::f32vec4(0.0, 0.0f, torusParameters.r1, 1.0f);
    for (float x = minCoord.x; x <= maxCoord.x; x += dx) {
        for (float z = minCoord.z; z <= maxCoord.z; z += dx) {
            // Find azimuthal angle
            float torusTheta = atan2(x, z);

            // Find closest torus centerpoint
            glm::mat4 xform = glm::mat4(1.0f);
            xform = glm::rotate(xform, torusTheta, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::f32vec4 nearestTorusCenter = xform * torusCenterPos;

            for (float y = minCoord.y; y <= maxCoord.y; y += dx) {
                float xDiff = x - nearestTorusCenter.x;
                float yDiff = y - nearestTorusCenter.y;
                float zDiff = z - nearestTorusCenter.z;

                bool isActive = xDiff*xDiff + yDiff*yDiff + zDiff*zDiff > torusParameters.r2*torusParameters.r2;

                Cell cell;
                cell.pos = glm::f32vec4 { x, y, z, isActive ? 1.0f : 0.0f };
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

glm::f32vec4 TokamakScene::rand_particle_position() {
    float r = rand_range(torusParameters.r1 - (torusParameters.r2 / 4.0f), torusParameters.r1 + (torusParameters.r2 / 4.0f));
    float theta = rand_range(0.0f, 2 * M_PI);
    float y = rand_range(-torusParameters.r2 / 4.0f, torusParameters.r2 / 4.0f);

    // [x, y, z, unused]
    return glm::f32vec4 { r * sin(theta), y, r * cos(theta), 0.0f };
}

bool TokamakScene::process_input(GLFWwindow* window, bool (*debounce_input)()) {
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && debounce_input()) {
        toggleShowTorus();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && debounce_input()) {
        toggleShowSolenoid();
        return true;
    }
    return Scene::process_input(window, debounce_input);
}

void TokamakScene::toggleShowTorus() {
    this->showTorus = !this->showTorus;
}

void TokamakScene::toggleShowSolenoid() {
    this->showSolenoid = !this->showSolenoid;
}