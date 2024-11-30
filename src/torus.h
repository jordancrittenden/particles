#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"
#include "cl_util.h"
#include "state.h"
#include "scene.h"
#include "current_segment.h"

typedef struct TorusParameters {
    float r1 = 1.0f;            // Radius of torus, m
    float r2 = 0.4f;            // Radius of torus cross section, m

    int toroidalCoils = 12;     // Number of toroidal coils
    int coilLoopSegments = 20;  // Number of current segments per circle for approximation
    float toroidalI = 50000.0f; // Toroidal current, A

    float solenoidR = 0.2f;     // Radius of the central solenoid, m
    float solenoidFlux = 0.3f;  // Central solenoid magnetic flux, V*s
} TorusParameters;

class TokamakScene : public Scene {
public:
    TokamakScene(SimulationState& state);
    ~TokamakScene();

    // Rendering
    void render(float aspectRatio);

    // Scene-dependent functions
    std::vector<Cell> get_grid_cells(float spacing);
    cl_float4 rand_particle_position();
    std::vector<CurrentVector> get_currents();

private:
    void render_torus(glm::mat4 view, glm::mat4 projection);
    GLBuffers create_torus_buffers();
    void generate_ring_vertices(std::vector<float>& vertices, std::vector<unsigned int>& indices);

    TorusParameters parameters;
    GLBuffers torusBuf;
    GLuint torusShaderProgram;
    bool showTorus = true;
};
