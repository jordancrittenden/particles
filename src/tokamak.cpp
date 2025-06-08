#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "tokamak.h"
#include "torus.h"
#include "geometry/ring.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

TokamakScene::TokamakScene(SimulationState& state, TorusParameters& params) : Scene(state), parameters(params) {
    torusShaderProgram = create_shader_program("shader/torus_vertex.glsl", "shader/torus_fragment.glsl"); 
}

void TokamakScene::initialize() {
    Scene::initialize();
    Ring ring;
    ring.r = parameters.r2;
    ring.t = 0.05f;
    ring.d = 0.1f;
    this->torusRingBuf = create_ring_buffers(ring);
    this->cameraDistance = 5.0f;
}

TokamakScene::~TokamakScene() {
    glDeleteVertexArrays(1, &this->torusRingBuf.vao);
}

void TokamakScene::render(float aspectRatio) {
    Scene::render(aspectRatio);
    if (this->showTorus) render_torus(torusShaderProgram, torusRingBuf, parameters, view, projection);
}

std::vector<Cell> TokamakScene::get_grid_cells(float dx) {
    std::vector<Cell> cells;

    glm::vec3 minCoord(-(parameters.r1 + parameters.r2), -parameters.r2, -(parameters.r1 + parameters.r2));
    glm::vec3 maxCoord(parameters.r1 + parameters.r2, parameters.r2, parameters.r1 + parameters.r2);

    glm::vec4 torusCenterPos = glm::vec4(0.0, 0.0f, parameters.r1, 1.0f);
    for (float x = minCoord.x; x <= maxCoord.x; x += dx) {
        for (float z = minCoord.z; z <= maxCoord.z; z += dx) {
            // Find azimuthal angle
            float torusTheta = atan2(x, z);

            // Find closest torus centerpoint
            glm::mat4 xform = glm::mat4(1.0f);
            xform = glm::rotate(xform, torusTheta, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec4 nearestTorusCenter = xform * torusCenterPos;

            for (float y = minCoord.y; y <= maxCoord.y; y += dx) {
                float xDiff = x - nearestTorusCenter.x;
                float yDiff = y - nearestTorusCenter.y;
                float zDiff = z - nearestTorusCenter.z;

                bool isActive = xDiff*xDiff + yDiff*yDiff + zDiff*zDiff > parameters.r2*parameters.r2;

                Cell cell;
                cell.pos = cl_float4 { x, y, z, isActive ? 1.0f : 0.0f };
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

cl_float4 TokamakScene::rand_particle_position() {
    float r = rand_range(parameters.r1 - (parameters.r2 / 2.0f), parameters.r1 + (parameters.r2 / 2.0f));
    float theta = rand_range(0.0f, 2 * M_PI);
    float y = rand_range(-parameters.r2 / 2.0f, parameters.r2 / 2.0f);

    // [x, y, z, unused]
    return cl_float4 { r * sin(theta), y, r * cos(theta), 0.0f };
}

std::vector<CurrentVector> TokamakScene::get_currents() {
    std::vector<CurrentVector> currents;

    std::vector<glm::vec4> circleVertices;
    float thetaStep = 2.0f * M_PI / parameters.coilLoopSegments;
    for (int i = 0; i < parameters.coilLoopSegments; i++) {
        float theta = i * thetaStep;
        circleVertices.push_back(glm::vec4 { parameters.r2 * cos(theta), parameters.r2 * sin(theta), 0.0f, 1.0f });
    }

    int idx = 0;
    int coilStartIdx = 0;
    for (int i = 0; i < parameters.toroidalCoils; ++i) {
        coilStartIdx = idx;

        float angle = (2.0f * M_PI * i) / parameters.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, parameters.r1);

        for (int j = 0; j < parameters.coilLoopSegments; ++j) {
            CurrentVector current;
            current.x = model * circleVertices[j];
            current.i = parameters.toroidalI;

            if (j > 0) currents[idx-1].dx = current.x - currents[idx-1].x;

            currents.push_back(current);
            idx++;
        }
        currents[idx-1].dx = currents[coilStartIdx].x - currents[idx-1].x;
    }

    return currents;
}