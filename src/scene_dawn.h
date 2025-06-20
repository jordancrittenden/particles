#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include "util/wgpu_util.h"
#include "render/axes_dawn.h"
#include "render/particles_dawn.h"
#include "state_dawn.h"

class Scene {
public:
    Scene(SimulationState& state);
    ~Scene();
    virtual void initialize(wgpu::Device& device);

    // Rendering
    virtual void render(wgpu::Device& device, wgpu::RenderPassEncoder& pass, float aspectRatio);

    // Scene-dependent functions
    virtual std::vector<Cell> get_grid_cells(float spacing);
    virtual glm::f32vec4 rand_particle_position();
    // virtual std::vector<CurrentVector> get_currents();
    virtual bool process_input(GLFWwindow* window, bool (*debounce_input)());

    // Toggles
    void toggleShowAxes();
    void toggleShowParticles();
    void toggleShowEField();
    void toggleShowBField();
    void toggleShowETracers();
    void toggleShowBTracers();

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
    
    ParticleBuffers particles;

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

    ParticleRender particleRender;
    AxesBuffers axes;

    // Field vectors

    // Tracer buffers
    // tracer.vbo: [
    //    trace0_x0, trace0_y0, trace0_z0, unused, trace0_x1, trace0_y1, trace0_z1, unused, ..., trace0_xEND, trace0_yEND, trace0_zEND, unused,
    //    trace1_x0, trace1_y0, trace1_z0, unused, trace1_x1, trace1_y1, trace1_z1, unused, ..., trace1_xEND, trace1_yEND, trace1_zEND, unused,
    //    ...
    //    traceN_x0, traceN_y0, traceN_z0, unused, traceN_x1, traceN_y1, traceN_z1, unused, ..., traceN_xEND, traceN_yEND, traceN_zEND, unused,
    // ]

    // Show/hide booleans
    bool showAxes = true;
    bool showParticles = true;
    bool showEField = false;
    bool showBField = false;
    bool showETracers = false;
    bool showBTracers = false;

    // Tracer settings
    int tracerPoints = 100;
    int nTracers = 0;
};