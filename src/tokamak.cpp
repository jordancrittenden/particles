#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "tokamak.h"
#include "render/torus.h"
#include "render/torus_structure.h"
#include "render/solenoid.h"
#include "render/ring.h"
#include "render/torus.h"
#include "emscripten_key.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min) + min;
}

TokamakScene::TokamakScene(const TorusParameters& params, const SolenoidParameters& solenoidParams) 
    : Scene(), torusParameters(params), solenoidParameters(solenoidParams) {
}

void TokamakScene::init(const SimulationParams& params) {
    Scene::init(params);

    // Create torus buffers
    Ring toroidalRing;
    toroidalRing.r = torusParameters.r2;
    toroidalRing.t = 0.05f * _M;
    toroidalRing.d = 0.1f * _M;
    this->torusBuf = create_torus_buffers(device, toroidalRing, torusParameters.toroidalCoils);

    // Create torus structure buffers
    this->torusStructureBuf = create_torus_structure_buffers(device, torusParameters.r1, torusParameters.r2, 64, 32);

    // Create solenoid buffers
    Ring solenoidRing;
    solenoidRing.r = solenoidParameters.r;
    solenoidRing.t = 0.05f * _M;
    solenoidRing.d = torusParameters.r2 * 2.0f;
    this->solenoidBuf = create_solenoid_buffers(device, solenoidRing);

    this->cameraDistance = 5.0f * _M;
}

void TokamakScene::render_details(wgpu::RenderPassEncoder& pass) {
    Scene::render_details(pass);
    if (this->showTorusStructure) {
        render_torus_structure(device, pass, torusStructureBuf, view, projection);
    }
    if (this->showTorus) {
        render_torus(device, pass, torusBuf, torusParameters.r1, this->toroidalI, view, projection);
    }
    if (this->showSolenoid) render_solenoid(device, pass, solenoidBuf, this->solenoidFlux, view, projection);
}

void TokamakScene::compute_field_step(wgpu::ComputePassEncoder& pass) {
    run_field_compute(
        device,
        pass,
        fieldCompute,
        static_cast<glm::u32>(cells.size()),
        static_cast<glm::u32>(cachedCurrents.size()),
        solenoidFlux,
        enableParticleFieldContributions);

    // Run tracer compute
    run_tracer_compute(
        device,
        pass,
        tracerCompute,
        dt,
        solenoidFlux,
        enableParticleFieldContributions,
        static_cast<glm::u32>(this->cachedCurrents.size()),
        nParticles,
        this->tracers.nTracers,
        TRACER_LENGTH);
}

std::vector<Cell> TokamakScene::get_mesh_cells(glm::f32vec3 size, MeshProperties& mesh) {
    std::vector<Cell> cells;

    glm::vec3 minCoord(-(torusParameters.r1 + torusParameters.r2), -torusParameters.r2, -(torusParameters.r1 + torusParameters.r2));
    glm::vec3 maxCoord(torusParameters.r1 + torusParameters.r2, torusParameters.r2, torusParameters.r1 + torusParameters.r2);

    glm::u32 nx = 0, ny = 0, nz = 0;
    bool countZ = true, countY = true;
    for (glm::f32 x = minCoord.x; x <= maxCoord.x; x += size.x) {
        for (glm::f32 z = minCoord.z; z <= maxCoord.z; z += size.z) {
            glm::f32 radialDistFromOrigin = sqrt(x*x + z*z);
            glm::f32 radialDistFromTorusCenterline = radialDistFromOrigin - torusParameters.r1;
            for (glm::f32 y = minCoord.y; y <= maxCoord.y; y += size.y) {
                glm::f32 distFromTorusCenterline = sqrt(radialDistFromTorusCenterline*radialDistFromTorusCenterline + y*y);
                bool isActive = distFromTorusCenterline < torusParameters.r2;

                Cell cell;
                cell.pos = glm::f32vec4 { x, y, z, isActive ? 1.0f : 0.0f };
                cell.min = glm::f32vec3 { x - size.x/2.0f, y - size.y/2.0f, z - size.z/2.0f };
                cell.max = glm::f32vec3 { x + size.x/2.0f, y + size.y/2.0f, z + size.z/2.0f };
                cells.push_back(cell);

                if (countY) ny++;
            }
            if (countZ) nz++;
            countY = false;
        }
        nx++;
        countZ = false;
    }
    mesh.dim = glm::u32vec3 { nx, ny, nz };
    mesh.cell_size = size;
    mesh.min = minCoord;
    mesh.max = maxCoord;
    
    return cells;
}

glm::f32vec4 TokamakScene::rand_particle_position() {
    float r = rand_range(torusParameters.r1 - (torusParameters.r2 / 4.0f), torusParameters.r1 + (torusParameters.r2 / 4.0f));
    float theta = rand_range(0.0f, 2 * M_PI);
    float y = rand_range(-torusParameters.r2 / 4.0f, torusParameters.r2 / 4.0f);

    // [x, y, z, unused]
    return glm::f32vec4 { r * sin(theta), y, r * cos(theta), 0.0f };
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
            current.i = toroidalI;

            if (j > 0) currents[idx-1].dx = current.x - currents[idx-1].x;

            currents.push_back(current);
            idx++;
        }
        currents[idx-1].dx = currents[coilStartIdx].x - currents[idx-1].x;
    }

    return currents;
}

bool TokamakScene::process_input(bool (*debounce_input)()) {
#if defined(__EMSCRIPTEN__)
    // Web keyboard input handling
    if (is_key_pressed(84) && debounce_input()) { // T key
        toggleShowTorus();
        return true;
    }
    if (is_key_pressed(85) && debounce_input()) { // U key
        toggleShowTorusStructure();
        return true;
    }
    if (is_key_pressed(83) && debounce_input()) { // S key
        toggleShowSolenoid();
        return true;
    }
    if (is_key_pressed(82) && debounce_input()) { // R key
        toggleEnableToroidalRings();
        return true;
    }
    if (is_key_pressed(70) && debounce_input()) { // F key
        toggleEnableSolenoidFlux();
        return true;
    }
#else
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && debounce_input()) {
        toggleShowTorus();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && debounce_input()) {
        toggleShowTorusStructure();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && debounce_input()) {
        toggleShowSolenoid();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && debounce_input()) {
        toggleEnableToroidalRings();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && debounce_input()) {
        toggleEnableSolenoidFlux();
        return true;
    }
#endif
    return Scene::process_input(debounce_input);
}

void TokamakScene::toggleShowTorus() {
    this->showTorus = !this->showTorus;
}

void TokamakScene::toggleShowTorusStructure() {
    this->showTorusStructure = !this->showTorusStructure;
}

void TokamakScene::toggleShowSolenoid() {
    this->showSolenoid = !this->showSolenoid;
}

void TokamakScene::toggleEnableToroidalRings() {
    this->refreshCurrents = true;
    this->enableToroidalRings = !this->enableToroidalRings;
    this->toroidalI = this->enableToroidalRings ? torusParameters.maxToroidalI : 0.0f;
    std::cout << "toroidal rings: " << (this->enableToroidalRings ? "ENABLED" : "DISABLED") << std::endl;
}

void TokamakScene::toggleEnableSolenoidFlux() {
    this->enableSolenoidFlux = !this->enableSolenoidFlux;
    this->solenoidFlux = this->enableSolenoidFlux ? solenoidParameters.maxSolenoidFlux : 0.0f;
    std::cout << "solenoid: " << (this->enableSolenoidFlux ? "ENABLED" : "DISABLED") << std::endl;
}