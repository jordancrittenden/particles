#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "tokamak.h"
#include "render/torus.h"
#include "render/solenoid.h"
#include "render/ring.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

TokamakScene::TokamakScene(SimulationState& state, TorusParameters& params, SolenoidParameters& solenoidParams) : Scene(state), torusParameters(params), solenoidParameters(solenoidParams) {
    torusShaderProgram = create_shader_program("shader/opengl/torus_vertex.glsl", "shader/opengl/torus_fragment.glsl");
    solenoidShaderProgram = create_shader_program("shader/opengl/solenoid_vertex.glsl", "shader/opengl/solenoid_fragment.glsl");
}

void TokamakScene::initialize() {
    Scene::initialize();

    Ring toroidalRing;
    toroidalRing.r = torusParameters.r2;
    toroidalRing.t = 0.05f * _M;
    toroidalRing.d = 0.1f * _M;
    this->torusRingBuf = create_ring_buffers(toroidalRing);

    Ring solenoidRing;
    solenoidRing.r = solenoidParameters.r;
    solenoidRing.t = 0.05f * _M;
    solenoidRing.d = torusParameters.r2 * 2.0f;
    this->solenoidRingBuf = create_ring_buffers(solenoidRing);

    this->cameraDistance = 5.0f * _M;
}

TokamakScene::~TokamakScene() {
    glDeleteVertexArrays(1, &this->torusRingBuf.vao);
    glDeleteVertexArrays(1, &this->solenoidRingBuf.vao);
}

void TokamakScene::render(float aspectRatio) {
    Scene::render(aspectRatio);
    if (this->showTorus) render_torus_rings(torusShaderProgram, torusRingBuf, torusParameters.toroidalCoils, torusParameters.r1, state->toroidalI, view, projection);
    if (this->showSolenoid) render_solenoid(solenoidShaderProgram, solenoidRingBuf, state->solenoidFlux, view, projection);
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
                cell.pos = cl_float4 { x, y, z, isActive ? 1.0f : 0.0f };
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

cl_float4 TokamakScene::rand_particle_position() {
    float r = rand_range(torusParameters.r1 - (torusParameters.r2 / 4.0f), torusParameters.r1 + (torusParameters.r2 / 4.0f));
    float theta = rand_range(0.0f, 2 * M_PI);
    float y = rand_range(-torusParameters.r2 / 4.0f, torusParameters.r2 / 4.0f);

    // [x, y, z, unused]
    return cl_float4 { r * sin(theta), y, r * cos(theta), 0.0f };
}

std::vector<CurrentVector> TokamakScene::get_currents() {
    std::vector<CurrentVector> currents;

    std::vector<glm::f32vec4> circleVertices;
    float thetaStep = 2.0f * M_PI / torusParameters.coilLoopSegments;
    for (int i = 0; i < torusParameters.coilLoopSegments; i++) {
        float theta = i * thetaStep;
        circleVertices.push_back(glm::f32vec4 { torusParameters.r2 * cos(theta), torusParameters.r2 * sin(theta), 0.0f, 1.0f });
    }

    int idx = 0;
    int coilStartIdx = 0;
    for (int i = 0; i < torusParameters.toroidalCoils; ++i) {
        coilStartIdx = idx;

        float angle = (2.0f * M_PI * i) / torusParameters.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, torusParameters.r1);

        for (int j = 0; j < torusParameters.coilLoopSegments; ++j) {
            CurrentVector current;
            current.x = model * circleVertices[j];
            current.i = state->toroidalI;

            if (j > 0) currents[idx-1].dx = current.x - currents[idx-1].x;

            currents.push_back(current);
            idx++;
        }
        currents[idx-1].dx = currents[coilStartIdx].x - currents[idx-1].x;
    }

    return currents;
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