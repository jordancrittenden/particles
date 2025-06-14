#pragma once

#include "state.h"
#include "util/gl_util.h"
#include "util/cl_util.h"
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
    cl::BufferGL getTracerBufCL(cl::Context* context);

    // Toggles
    void toggleShowAxes();
    void toggleShowParticles();
    void toggleShowEField();
    void toggleShowBField();
    void toggleShowTracers();

    // Camera
    void zoomIn();
    void zoomOut();
    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();

    // Misc
    int getTracerPoints();
    int getNumTracers();

protected:
    SimulationState* state;

    // Camera settings
    float cameraDistance = 5.0f * _M;
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
    GLuint tracerShaderProgram;

    // Particle state buffers
    GLBuffers pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBuffers vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBuffers axes;

    // Field vectors
    FieldGLBuffers e_field;
    FieldGLBuffers b_field;

    // Tracer buffers
    // tracer.vbo: [
    //    trace0_x0, trace0_y0, trace0_z0, unused, trace0_x1, trace0_y1, trace0_z1, unused, ..., trace0_xEND, trace0_yEND, trace0_zEND, unused,
    //    trace1_x0, trace1_y0, trace1_z0, unused, trace1_x1, trace1_y1, trace1_z1, unused, ..., trace1_xEND, trace1_yEND, trace1_zEND, unused,
    //    ...
    //    traceN_x0, traceN_y0, traceN_z0, unused, traceN_x1, traceN_y1, traceN_z1, unused, ..., traceN_xEND, traceN_yEND, traceN_zEND, unused,
    // ]
    GLBuffers tracers;

    // Show/hide booleans
    bool showAxes = true;
    bool showParticles = true;
    bool showEField = false;
    bool showBField = false;
    bool showTracers = true;

    // Tracer settings
    int tracerPoints = 100;
    int nTracers = 0;
};