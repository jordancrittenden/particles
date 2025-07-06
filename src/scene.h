#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>
#include "util/wgpu_util.h"
#include "shared/particles.h"
#include "shared/fields.h"
#include "shared/tracers.h"
#include "render/axes.h"
#include "render/cell_box.h"
#include "render/particles.h"
#include "render/spheres.h"
#include "render/fields.h"
#include "render/tracers.h"
#include "compute/particles.h"
#include "compute/fields.h"
#include "compute/tracers.h"
#include "current_segment.h"
#include "cell.h"
#include "args.h"

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

class Scene {
public:
    virtual void init(const SimulationParams& params);
    void run_once();
    void render();
    void compute();
    void terminate();
    bool is_running();

    // Scene-dependent functions
    virtual std::vector<Cell> get_grid_cells(glm::f32 spacing, glm::u32& nx, glm::u32& ny, glm::u32& nz);
    virtual glm::f32vec4 rand_particle_position();
    virtual std::vector<CurrentVector> get_currents();
    virtual bool process_input(bool (*debounce_input)());

    // Toggles
    void toggleShowAxes();
    void toggleShowParticles();
    void toggleShowEField();
    void toggleShowBField();
    void toggleShowETracers();
    void toggleShowBTracers();
    void toggleShowCellBoxes();
    void toggleRenderParticlesAsSpheres();
    
    // Camera
    void zoomIn();
    void zoomOut();
    void rotateLeft();
    void rotateRight();
    void rotateUp();
    void rotateDown();

protected:
    virtual void render_details(wgpu::RenderPassEncoder& pass);
    virtual void compute_step(wgpu::ComputePassEncoder& pass);

    bool refreshCurrents = false;
    
    // Camera settings
    float cameraDistance = 5.0f * _M;
    float cameraTheta = 5.0f/6.0f * M_PI_2;
    float cameraPhi = 1.0f/6.0f * M_PI_2;
    glm::mat4 view;
    glm::mat4 projection;

    // Simulation cells
    std::vector<Cell> cells;
    glm::u32 cellsXSize = 0;
    glm::u32 cellsYSize = 0;
    glm::u32 cellsZSize = 0;

    // Physics state variables
    glm::f32 t = 0.0f * _S;    // Simulation time, s
    glm::f32 dt = 1e-8f * _S;  // Simulation dt, s
    bool enableParticleFieldContributions = false;

    ParticleCompute particleCompute;
    FieldCompute fieldCompute;
    TracerCompute tracerCompute;
    TracerBuffers tracers;
    glm::u32 nParticles;

    // Currents
    std::vector<CurrentVector> cachedCurrents;
    wgpu::Buffer currentSegmentsBuffer;

    // WebGPU objects
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Surface surface;
    wgpu::TextureFormat format;

    // GLFW window
    GLFWwindow* window = nullptr;
    glm::u32 windowWidth = 1024;
    glm::u32 windowHeight = 768;
    float targetFPS = 60.0f;

private:
    void init_webgpu();
    glm::mat4 get_orbit_view_matrix();

    // Particles
    ParticleBuffers particles;
    ParticleRender particleRender;
    SphereRender sphereRender;
    
    // Field vectors
    FieldBuffers fields;
    FieldRender eFieldRender;
    FieldRender bFieldRender;

	// Axes
    AxesBuffers axes;

    // Cell boxes
    CellBoxBuffers cellBoxes;

    // Tracers
    TracerRender eTracerRender;
    TracerRender bTracerRender;

    // Show/hide booleans
    bool showAxes = true;
    bool showParticles = true;
    bool showEField = false;
    bool showBField = false;
    bool showETracers = true;
    bool showBTracers = true;
    bool showCellBoxes = true;
    bool renderParticlesAsSpheres = false;
    
    int frameCount = 0;
    int simulationStep = 0;
};