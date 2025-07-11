#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "shared/particles.h"
#include "shared/fields.h"
#include "shared/tracers.h"
#include "render/axes.h"
#include "render/particles.h"
#include "render/fields.h"
#include "render/tracers.h"
#include "compute/particles.h"
#include "compute/fields.h"
#include "compute/tracers.h"
#include "current_segment.h"
#include "scene.h"
#include "free_space.h"
#include "plasma.h"
#include "mesh.h"
#include "emscripten_key.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min) + min;
}

void Scene::init_webgpu() {
    wgpu::InstanceDescriptor instanceDesc{.capabilities = {.timedWaitAnyEnable = true}};
    instance = wgpu::CreateInstance(&instanceDesc);

    wgpu::Future f1 = instance.RequestAdapter(
        nullptr,
        wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cout << "RequestAdapter: " << message.data << "\n";
                exit(0);
            }
            adapter = std::move(a);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::Limits limits;
    limits.maxStorageBufferBindingSize = 2ull * 1024 * 1024 * 1024; // 2GB

    wgpu::DeviceDescriptor desc{};
    desc.requiredLimits = &limits;
    desc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
        std::cout << "Error: " << static_cast<uint32_t>(errorType) << " - message: " << message.data << "\n";
    });

    wgpu::Future f2 = adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cout << "RequestDevice: " << message.data << "\n";
                exit(0);
            }
            device = std::move(d);
        });
    instance.WaitAny(f2, UINT64_MAX);

#if defined(__EMSCRIPTEN__)
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector src{{.selector = "#canvas"}};
    wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &src};
    surface = instance.CreateSurface(&surfaceDesc);
    
    // Set up keyboard event handlers for web
    emscripten_set_keydown_callback("#canvas", nullptr, 1, keydown_callback);
    emscripten_set_keyup_callback("#canvas", nullptr, 1, keyup_callback);
#else
    if (!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(windowWidth, windowHeight, "Plasma Simulation", nullptr, nullptr);
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif

    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    format = capabilities.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = format,
        .width = windowWidth,
        .height = windowHeight,
        .presentMode = wgpu::PresentMode::Fifo
    };
    surface.Configure(&config);
}

bool Scene::is_running() {
    return !glfwWindowShouldClose(window);
}

void Scene::terminate() {
#if !defined(__EMSCRIPTEN__)
    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}

void Scene::init(const SimulationParams& params) {
    this->windowWidth = params.windowWidth;
    this->windowHeight = params.windowHeight;
    this->init_webgpu();

    // Initialize cells
    this->cells = get_mesh_cells(glm::f32vec3(params.cellSpacing), this->mesh);
    std::vector<bool> cellBoxesVisible;
    cellBoxesVisible.reserve(cells.size());
    glm::u32 activeCells = 0;
    for (auto& cell : cells) {
        cellBoxesVisible.push_back(cell.pos.w > 0.0f);
        if (cell.pos.w > 0.0f) activeCells++;
    }
    std::cout << "Simulation cells: " << cells.size() << " (active: " << activeCells << ")" << std::endl;

    // Initialize axes
    this->axes = create_axes_buffers(device);
    this->cameraDistance = 0.5f * _M;

    // Initialize particles
    this->nParticles = params.initialParticles;
	this->particles = create_particle_buffers(
        device,
        [this](){ return rand_particle_position(); },
        [&params](PARTICLE_SPECIES species){ return maxwell_boltzmann_particle_velocty(params.initialTemperature, particle_mass(species)); },
        [](){ return rand_particle_species(0.0f, 0.0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f); },
        params.initialParticles,
        params.maxParticles);
    this->particleRender = create_particle_render(device);
    this->sphereRender = create_sphere_render(device);

    // Initialize field vectors
    std::vector<glm::f32vec4> eFieldLoc, eFieldVec;
    std::vector<glm::f32vec4> bFieldLoc, bFieldVec;
    for (auto& cell : cells) {
        eFieldLoc.push_back(glm::f32vec4(cell.pos.x, cell.pos.y, cell.pos.z, 0.0f)); // Last element indicates E vs B
        eFieldVec.push_back(glm::f32vec4(1.0f, 0.0f, 0.0f, 0.0f));  // initial (meaningless) value
        bFieldLoc.push_back(glm::f32vec4(cell.pos.x, cell.pos.y, cell.pos.z, 1.0f)); // Last element indicates E vs B
        bFieldVec.push_back(glm::f32vec4(-1.0f, 1.0f, 0.0f, 0.0f)); // initial (meaningless) value
    }
    this->fields = create_fields_buffers(device, cells.size());
    this->eFieldRender = create_fields_render(device, eFieldLoc, eFieldVec, params.cellSpacing / 2.0f);
    this->bFieldRender = create_fields_render(device, bFieldLoc, bFieldVec, params.cellSpacing / 2.0f);

    // Initialize cell boxes
    this->cellBoxes = create_cell_box_buffers(device, cells, params.cellSpacing);
    update_cell_visibility(device, cellBoxes, cells, cellBoxesVisible);

    // Initialize tracers
    std::vector<glm::f32vec4> tracerLoc;
    for (int i = 0; i < cells.size(); i++) {
        if (rand_range(0.0f, 100.0f) < params.tracerDensity) {
            tracerLoc.push_back(glm::f32vec4(cells[i].pos.x, cells[i].pos.y, cells[i].pos.z, 0.0f));
        }
    }
    this->tracers = create_tracer_buffers(device, tracerLoc);
    this->eTracerRender = create_tracer_render(device);
    this->bTracerRender = create_tracer_render(device);

    // Initialize currents
    this->cachedCurrents = get_currents();
	this->currentSegmentsBuffer = get_current_segment_buffer(device, this->cachedCurrents);

    // Initialize particle compute
	this->particleCompute = create_particle_pic_compute(device, particles, fields, params.maxParticles);

    // Initialize field compute    
    this->fieldCompute = create_field_compute(device, cells, particles, fields, this->currentSegmentsBuffer, static_cast<glm::u32>(this->cachedCurrents.size()), params.maxParticles);

    // Initialize tracer compute
    this->tracerCompute = create_tracer_compute(device, tracers, particles, this->currentSegmentsBuffer, static_cast<glm::u32>(this->cachedCurrents.size()), params.maxParticles);
}

glm::mat4 Scene::get_orbit_view_matrix() {
    float cameraX = this->cameraDistance * sin(this->cameraTheta) * sin(this->cameraPhi);
    float cameraY = this->cameraDistance * cos(this->cameraTheta);
    float cameraZ = this->cameraDistance * sin(this->cameraTheta) * cos(this->cameraPhi);
    return glm::lookAt(
        glm::vec3(cameraX, cameraY, cameraZ), // eye
        glm::vec3(0.0f, 0.0f, 0.0f),          // target
        glm::vec3(0.0f, 1.0f, 0.0f)           // up
    );
}

auto lastInputTime = std::chrono::high_resolution_clock::now();
bool debounce_input() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - lastInputTime;
    if (diff.count() < 0.1) return false;
    lastInputTime = now;
    return true;
}

void Scene::run_once() {
    auto now = std::chrono::high_resolution_clock::now();

    // Render a frame
    render();

    // Compute until the next frame
    auto frameDur = std::chrono::high_resolution_clock::now() - now;
    do {
        compute();
        frameDur = std::chrono::high_resolution_clock::now() - now;
    } while (frameDur.count() < (1.0f / targetFPS));
}

void Scene::render() {
    // Process keyboard input
    glfwPollEvents();
    this->process_input(debounce_input);
    poll_events(device, false);

    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment {
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = {1.0f, 1.0f, 1.0f, 1.0f}  // White background
    };

    wgpu::TextureDescriptor depthTextureDesc {
        .usage = wgpu::TextureUsage::RenderAttachment,
        .size = {windowWidth, windowHeight},
        .format = wgpu::TextureFormat::Depth24Plus
    };
    wgpu::Texture depthTexture = device.CreateTexture(&depthTextureDesc);

    wgpu::RenderPassDepthStencilAttachment depthAttachment {
        .view = depthTexture.CreateView(),
        .depthLoadOp = wgpu::LoadOp::Clear,
        .depthStoreOp = wgpu::StoreOp::Store,
        .depthClearValue = 1.0
    };

    wgpu::RenderPassDescriptor renderPass{
        .label = "Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &attachment,
        .depthStencilAttachment = &depthAttachment
    };

    wgpu::CommandEncoderDescriptor encoderDesc{.label = "Render Command Encoder"};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);

    this->render_details(pass);

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

#if !defined(__EMSCRIPTEN__)
    surface.Present();
#endif
    instance.ProcessEvents();

    frameCount++;
}

void Scene::render_details(wgpu::RenderPassEncoder& pass) {
    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    this->view = get_orbit_view_matrix();
    this->projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    if (this->showAxes)      render_axes(device, pass, axes, view, projection);
    if (this->showParticles) {
        if (this->renderParticlesAsSpheres) {
            render_particles_as_spheres(device, pass, particles, sphereRender, nParticles, view, projection);
        } else {
            render_particles(device, pass, particles, particleRender, nParticles, view, projection);
        }
    }
    if (this->showEField)    render_fields(device, pass, eFieldRender, fields.eField, cells.size(), view, projection);
    if (this->showBField)    render_fields(device, pass, bFieldRender, fields.bField, cells.size(), view, projection);
    if (this->showETracers)  render_e_tracers(device, pass, tracers, eTracerRender, view, projection);
    if (this->showBTracers)  render_b_tracers(device, pass, tracers, bTracerRender, view, projection);
    if (this->showCellBoxes) render_cell_boxes(device, pass, cellBoxes, view, projection);
}

void Scene::compute() {
    // Update currents in scene
    if (this->refreshCurrents) {
        this->cachedCurrents = get_currents();
        update_currents_buffer(device, this->currentSegmentsBuffer, this->cachedCurrents);
        this->refreshCurrents = false;
    }

    wgpu::CommandEncoderDescriptor encoderDesc{.label = "Compute Command Encoder"};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
    wgpu::ComputePassDescriptor computePassDesc{.label = "Compute Pass"};
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);

    this->compute_field_step(pass);

    run_particle_pic_compute(
        device,
        pass,
        particleCompute,
        mesh,
        dt,
        nParticles);

    this->compute_wall_interactions(pass);

    pass.End();
    
    encoder.CopyBufferToBuffer(particles.nCur, 0, particleCompute.nParticlesReadBuf, 0, sizeof(glm::u32));
    encoder.CopyBufferToBuffer(particleCompute.debugStorageBuf, 0, particleCompute.debugReadBuf, 0, 10 * sizeof(glm::f32vec4));
    encoder.CopyBufferToBuffer(tracerCompute.eDebugStorageBuf, 0, tracerCompute.eDebugReadBuf, 0, 10 * sizeof(glm::f32vec4));
    encoder.CopyBufferToBuffer(tracerCompute.bDebugStorageBuf, 0, tracerCompute.bDebugReadBuf, 0, 10 * sizeof(glm::f32vec4));

    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    // std::vector<glm::f32vec4> debug;
    // read_particles_debug(device, instance, particleCompute, debug, 1000);

    if (simulationStep % 5000 == 0) {
        std::cout << "SIM STEP " << simulationStep << " (frame " << frameCount << ") [" << nParticles << " particles]" << std::endl;
    }
    simulationStep++;

    t += dt;
}

void Scene::compute_field_step(wgpu::ComputePassEncoder& pass) {
    run_field_compute(
        device,
        pass,
        fieldCompute,
        static_cast<glm::u32>(cells.size()),
        static_cast<glm::u32>(cachedCurrents.size()),
        0.0f,
        enableParticleFieldContributions);

    // Run tracer compute
    run_tracer_compute(
        device,
        pass,
        tracerCompute,
        dt,
        0.0f,
        enableParticleFieldContributions,
        static_cast<glm::u32>(cachedCurrents.size()),
        nParticles,
        tracers.nTracers,
        TRACER_LENGTH);
}

void Scene::compute_wall_interactions(wgpu::ComputePassEncoder& pass) {
    // TODO: Implement wall interactions
}

std::vector<Cell> Scene::get_mesh_cells(glm::f32vec3 size, MeshProperties& mesh) {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return get_free_space_mesh_cells(minCoord, maxCoord, size, mesh);
}

glm::f32vec4 Scene::rand_particle_position() {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return free_space_rand_particle_position(minCoord, maxCoord);
}

std::vector<CurrentVector> Scene::get_currents() {
    std::vector<CurrentVector> currents;
    // Dummy current (compute shader fails if currents is empty)
    currents.push_back(CurrentVector {
        .x = glm::f32vec4(0.0f, 0.0f, 0.0f, 0.0f),
        .dx = glm::f32vec4(1.0f, 0.0f, 0.0f, 0.0f),
        .i = 0.0f
    });
    return currents;
}

bool Scene::process_input(bool (*debounce_input)()) {
#if defined(__EMSCRIPTEN__)
    // Web keyboard input handling
    if (is_key_pressed(187) && debounce_input()) { // EQUAL key (=)
        dt *= 1.2f;
        std::cout << "dt: " << dt << std::endl;
    }
    if (is_key_pressed(189) && debounce_input()) { // MINUS key (-)
        dt /= 1.2f;
        std::cout << "dt: " << dt << std::endl;
    }
    if (is_key_pressed(73) && debounce_input()) { // I key
        enableParticleFieldContributions = !enableParticleFieldContributions;
        std::cout << "particle field contributions: " << (enableParticleFieldContributions ? "ENABLED" : "DISABLED") << std::endl;
    }
    if (is_key_pressed(37)) { // LEFT ARROW
        rotateLeft();
        return true;
    }
    if (is_key_pressed(39)) { // RIGHT ARROW
        rotateRight();
        return true;
    }
    if (is_key_pressed(40)) { // DOWN ARROW
        rotateDown();
        return true;
    }
    if (is_key_pressed(38)) { // UP ARROW
        rotateUp();
        return true;
    }
    if (is_key_pressed(219)) { // LEFT BRACKET [
        zoomOut();
        return true;
    }
    if (is_key_pressed(221)) { // RIGHT BRACKET ]
        zoomIn();
        return true;
    }
    if (is_key_pressed(65) && debounce_input()) { // A key
        toggleShowAxes();
        return true;
    }
    if (is_key_pressed(69) && debounce_input()) { // E key
        toggleShowEField();
        return true;
    }
    if (is_key_pressed(66) && debounce_input()) { // B key
        toggleShowBField();
        return true;
    }
    if (is_key_pressed(81) && debounce_input()) { // Q key
        toggleShowETracers();
        return true;
    }
    if (is_key_pressed(87) && debounce_input()) { // W key
        toggleShowBTracers();
        return true;
    }
    if (is_key_pressed(80) && debounce_input()) { // P key
        toggleShowParticles();
        return true;
    }
    if (is_key_pressed(79) && debounce_input()) { // O key
        toggleRenderParticlesAsSpheres();
        return true;
    }
    if (is_key_pressed(67) && debounce_input()) { // C key
        toggleShowCellBoxes();
        return true;
    }
#else
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && debounce_input()) {
        dt *= 1.2f;
        std::cout << "dt: " << dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && debounce_input()) {
        dt /= 1.2f;
        std::cout << "dt: " << dt << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && debounce_input()) {
        enableParticleFieldContributions = !enableParticleFieldContributions;
        std::cout << "particle field contributions: " << (enableParticleFieldContributions ? "ENABLED" : "DISABLED") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        rotateLeft();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rotateRight();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        rotateDown();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        rotateUp();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        zoomOut();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        zoomIn();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && debounce_input()) {
        toggleShowAxes();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && debounce_input()) {
        toggleShowEField();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && debounce_input()) {
        toggleShowBField();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && debounce_input()) {
        toggleShowETracers();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && debounce_input()) {
        toggleShowBTracers();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && debounce_input()) {
        toggleShowParticles();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && debounce_input()) {
        toggleRenderParticlesAsSpheres();
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && debounce_input()) {
        toggleShowCellBoxes();
        return true;
    }
#endif
    return false;
}

void Scene::toggleShowAxes() {
    this->showAxes = !this->showAxes;
    std::cout << "Axes: " << (this->showAxes ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowParticles() {
    this->showParticles = !this->showParticles;
    std::cout << "Particles: " << (this->showParticles ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowEField() {
    this->showEField = !this->showEField;
    std::cout << "E field: " << (this->showEField ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowBField() {
    this->showBField = !this->showBField;
    std::cout << "B field: " << (this->showBField ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowETracers() {
    this->showETracers = !this->showETracers;
    std::cout << "E tracers: " << (this->showETracers ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowBTracers() {
    this->showBTracers = !this->showBTracers;
    std::cout << "B tracers: " << (this->showBTracers ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleShowCellBoxes() {
    this->showCellBoxes = !this->showCellBoxes;
    std::cout << "Cell boxes: " << (this->showCellBoxes ? "VISIBLE" : "HIDDEN") << std::endl;
}

void Scene::toggleRenderParticlesAsSpheres() {
    this->renderParticlesAsSpheres = !this->renderParticlesAsSpheres;
    std::cout << "Particle rendering mode: " << (this->renderParticlesAsSpheres ? "SPHERES" : "POINTS") << std::endl;
}

void Scene::zoomIn() {
    this->cameraDistance = std::max(0.0f, this->cameraDistance / 1.01f);
}

void Scene::zoomOut() {
    this->cameraDistance = std::min(10.0f * _M, this->cameraDistance * 1.01f);
}

void Scene::rotateLeft() {
    this->cameraPhi -= 0.01f;
}

void Scene::rotateRight() {
    this->cameraPhi += 0.01f;
}

void Scene::rotateUp() {
    this->cameraTheta = std::max(0.001f, this->cameraTheta - 0.01f);
}

void Scene::rotateDown() {
    this->cameraTheta = std::min((float)M_PI, this->cameraTheta + 0.01f);
}