#pragma once

#include "util/gl_util.h"
#include "current_segment.h"
#include "scene.h"

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
    TokamakScene(SimulationState& state, TorusParameters& params, SolenoidParameters& solenoidParams);
    ~TokamakScene();
    void initialize();

    // Rendering
    void render(float aspectRatio);

    // Scene-dependent functions
    std::vector<Cell> get_grid_cells(float spacing);
    cl_float4 rand_particle_position();
    std::vector<CurrentVector> get_currents();

private:
    const TorusParameters& torusParameters;
    const SolenoidParameters& solenoidParameters;
    GLBuffers torusRingBuf;
    GLBuffers solenoidRingBuf;
    GLuint torusShaderProgram;
    GLuint solenoidShaderProgram;
    bool showTorus = true;
    bool showSolenoid = true;
};