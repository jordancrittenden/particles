#pragma once

#include "util/gl_util.h"
#include "current_segment.h"
#include "scene.h"
#include "torus.h"
#include "solenoid.h"
#include "render/ring.h"

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