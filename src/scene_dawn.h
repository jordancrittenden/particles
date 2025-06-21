#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include "util/wgpu_util.h"
#include "render/axes_dawn.h"
#include "shared/particles.h"
#include "render/particles_dawn.h"
#include "compute/particles.h"
#include "current_segment_dawn.h"
#include "cell.h"
#include "args_dawn.h"

class Scene {
public:
    virtual void initialize(wgpu::Device& device, const SimulationParams& params);

    // Rendering
    virtual void render(wgpu::Device& device, wgpu::RenderPassEncoder& pass, float aspectRatio);

    // Compute
    virtual void compute_step(wgpu::Device& device, wgpu::ComputePassEncoder pass);
    virtual void compute_copy(wgpu::CommandEncoder& encoder);
    virtual void compute_read(wgpu::Device& device, wgpu::Instance& instance);

    // Scene-dependent functions
    virtual std::vector<Cell> get_grid_cells(glm::f32 spacing);
    virtual glm::f32vec4 rand_particle_position();
    virtual std::vector<CurrentVector> get_currents();
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

    // Particle buffers
    ParticleBuffers particles;
    glm::u32 nParticles;

    // Simulation cells
    std::vector<Cell> cells;

protected:
    bool refreshCurrents = false;
    
    // Camera settings
    float cameraDistance = 5.0f * _M;
    float cameraTheta = 5.0f/6.0f * M_PI_2;
    float cameraPhi = 1.0f/6.0f * M_PI_2;
    glm::mat4 view;
    glm::mat4 projection;

private:
    glm::mat4 get_orbit_view_matrix();

    ParticleCompute compute;
    ParticleRender particleRender;
    AxesBuffers axes;

    std::vector<CurrentVector> cachedCurrents;
    wgpu::Buffer currentSegmentsBuffer;

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

    // Physics state variables
    glm::f32 t = 0.0f * _S;    // Simulation time, s
    glm::f32 dt = 1e-8f * _S;  // Simulation dt, s
    bool enableParticleFieldContributions = false;

    // Tracer settings
    int tracerPoints = 100;
    int nTracers = 0;
};