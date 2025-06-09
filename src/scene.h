#pragma once

#include "state.h"
#include "gl_util.h"
#include "cl_util.h"
#include "geometry/field_vector.h"
#include "current_segment.h"

class Scene {
public:
    Scene(SimulationState& state);
    ~Scene();
    virtual void initialize();

    // Rendering
    virtual void render(float aspectRatio);

    // Scene-dependent functions
    virtual std::vector<Cell> get_grid_cells(float spacing);
    virtual cl_float4 rand_particle_position();
    virtual std::vector<CurrentVector> get_currents();

    // OpenCL buffer accessors
    cl::BufferGL getParticlePosBufCL(cl::Context* context);
    cl::BufferGL getParticleVelBufCL(cl::Context* context);
    cl::BufferGL getEFieldVecBufCL(cl::Context* context);
    cl::BufferGL getBFieldVecBufCL(cl::Context* context);

    // Toggles
    void toggleShowAxes();
    void toggleShowParticles();
    void toggleShowEField();
    void toggleShowBField();

    // Camera
    void zoomIn();
    void zoomOut();
    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();

protected:
    SimulationState* state;

    // Camera settings
    float cameraDistance = 5.0f;
    float cameraTheta = 5.0f/6.0f * M_PI_2;
    float cameraPhi = 1.0f/6.0f * M_PI_2;
    glm::mat4 view;
    glm::mat4 projection;

private:
    glm::mat4 get_orbit_view_matrix();

    // Shader programs
    GLuint axesShaderProgram;
    GLuint particlesShaderProgram;
    GLuint vectorShaderProgram;

    // Particle state buffers
    GLBuffers pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBuffers vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBuffers axes;

    // Field vectors
    FieldGLBuffers e_field;
    FieldGLBuffers b_field;

    // Show/hide booleans
    bool showAxes = true;
    bool showParticles = true;
    bool showEField = false;
    bool showBField = false;
};