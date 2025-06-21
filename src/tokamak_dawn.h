#pragma once

#include "util/wgpu_util.h"
#include "scene_dawn.h"
#include "render/torus_dawn.h"
#include "render/solenoid_dawn.h"
#include "args_dawn.h"

typedef struct TorusParameters {
    float r1 = 1.0f * _M;                   // Radius of torus, m
    float r2 = 0.4f * _M;                   // Radius of torus cross section, m

    int toroidalCoils = 12;                 // Number of toroidal coils
    int coilLoopSegments = 20;              // Number of current segments per circle for approximation
    float maxToroidalI = 50000.0f * _A;     // Maximum current through the toroidal coils, A
} TorusParameters;


typedef struct SolenoidParameters {
    float r = 0.15f * _M;                   // Radius of the central solenoid, m
    float maxSolenoidFlux = 0.3f * _V * _S; // Maximum central solenoid magnetic flux, V s
} SolenoidParameters;


class TokamakScene : public Scene {
public:
    TokamakScene(const TorusParameters& params, const SolenoidParameters& solenoidParams);

    void initialize(wgpu::Device& device, const SimulationParams& params) override;

    // Rendering
    void render(wgpu::Device& device, wgpu::RenderPassEncoder& pass, float aspectRatio) override;

    // Compute
    void compute_step(wgpu::Device& device, wgpu::ComputePassEncoder pass) override;
    void compute_copy(wgpu::CommandEncoder& encoder) override;
    void compute_read(wgpu::Device& device, wgpu::Instance& instance) override;

    // Scene-dependent functions
    std::vector<Cell> get_grid_cells(glm::f32 spacing) override;
    glm::f32vec4 rand_particle_position() override;
    std::vector<CurrentVector> get_currents() override;
    bool process_input(GLFWwindow* window, bool (*debounce_input)()) override;

    // Toggles
    void toggleShowTorus();
    void toggleShowSolenoid();
    void toggleEnableToroidalRings();
    void toggleEnableSolenoidFlux();

private:
    const TorusParameters& torusParameters;
    const SolenoidParameters& solenoidParameters;

    TorusBuffers torusBuf;
    SolenoidBuffers solenoidBuf;

    bool showTorus = true;
    bool showSolenoid = true;

    glm::f32 solenoidFlux = 0.0f * _V * _S; // Flux through the solenoid, V s
    glm::f32 toroidalI = 50000.0f * _A;     // Current through the toroidal coils, A

    bool enableToroidalRings = true;
    bool enableSolenoidFlux = false;
};