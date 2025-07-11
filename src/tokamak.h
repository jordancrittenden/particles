#pragma once

#include "util/wgpu_util.h"
#include "scene.h"
#include "render/coils.h"
#include "render/torus.h"
#include "render/solenoid.h"
#include "args.h"

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

    // Initialization
    void init(const SimulationParams& params) override;

    // Rendering
    void render_details(wgpu::RenderPassEncoder& pass) override;

    // Compute
    void compute_field_step(wgpu::ComputePassEncoder& pass) override;

    // Scene-dependent functions
    std::vector<Cell> get_mesh_cells(glm::f32vec3 size, MeshProperties& mesh) override;
    glm::f32vec4 rand_particle_position() override;
    std::vector<CurrentVector> get_currents() override;
    bool process_input(bool (*debounce_input)()) override;

    // Toggles
    void toggleShowTorus();
    void toggleShowCoils();
    void toggleShowSolenoid();
    void toggleEnableToroidalRings();
    void toggleEnableSolenoidFlux();

private:
    const TorusParameters& torusParameters;
    const SolenoidParameters& solenoidParameters;

    CoilsBuffers coilsBuf;
    TorusBuffers torusBuf;
    SolenoidBuffers solenoidBuf;

    bool showTorus = true;
    bool showCoils = true;
    bool showSolenoid = true;
    bool enableToroidalRings = true;
    bool enableSolenoidFlux = false;

    glm::f32 solenoidFlux = 0.0f * _V * _S; // Flux through the solenoid, V s
    glm::f32 toroidalI = 50000.0f * _A;     // Current through the toroidal coils, A
};