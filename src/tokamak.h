#pragma once

#include "gl_util.h"
#include "current_segment.h"
#include "scene.h"
#include "torus.h"

class TokamakScene : public Scene {
public:
    TokamakScene(SimulationState& state, TorusParameters& params);
    ~TokamakScene();
    void initialize();

    // Rendering
    void render(float aspectRatio);

    // Scene-dependent functions
    std::vector<Cell> get_grid_cells(float spacing);
    cl_float4 rand_particle_position();
    std::vector<CurrentVector> get_currents();

private:
    const TorusParameters parameters;
    GLBuffers torusBuf;
    GLuint torusShaderProgram;
    bool showTorus = true;
};