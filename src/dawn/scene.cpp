#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "shared/particles.h"
#include "shared/fields.h"
#include "render/axes.h"
#include "render/particles.h"
#include "render/fields.h"
#include "compute/particles.h"
#include "compute/fields.h"
#include "current_segment.h"
#include "scene.h"
#include "free_space.h"
#include "plasma.h"
#include "cell.h"

void Scene::initialize(wgpu::Device& device, const SimulationParams& params) {
    this->cells = get_grid_cells(params.cellSpacing);
    std::cout << "Simulation cells: " << cells.size() << std::endl;

    this->axes = create_axes_buffers(device);
    this->cameraDistance = 0.5f * _M;

    // Initialize particles
    this->nParticles = params.initialParticles;
	this->particles = create_particle_buffers(
        device,
        [this](){ return rand_particle_position(); },
        [&params](PARTICLE_SPECIES species){ return maxwell_boltzmann_particle_velocty(params.initialTemperature, particle_mass(species)); },
        [](){ return rand_particle_species(0.0f, 0.3f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f); },
        params.initialParticles,
        params.maxParticles);
    this->particleRender = create_particle_render(device);

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
    this->eFieldRender = create_fields_render(device, eFieldLoc, eFieldVec, 0.03f * _M);
    this->bFieldRender = create_fields_render(device, bFieldLoc, bFieldVec, 0.03f * _M);

    // Initialize currents
    this->cachedCurrents = get_currents();
	this->currentSegmentsBuffer = get_current_segment_buffer(device, this->cachedCurrents);

    // Initialize particle compute
	this->particleCompute = create_particle_compute(device, particles, this->currentSegmentsBuffer, static_cast<glm::u32>(this->cachedCurrents.size()), params.maxParticles);

    // Initialize field compute    
    this->fieldCompute = create_field_compute(device, cells, particles, fields, this->currentSegmentsBuffer, static_cast<glm::u32>(this->cachedCurrents.size()), params.maxParticles);
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

void Scene::render(wgpu::Device& device, wgpu::RenderPassEncoder& pass, float aspectRatio) {
    this->view = get_orbit_view_matrix();
    this->projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    if (this->showAxes)      render_axes(device, pass, axes, view, projection);
    if (this->showParticles) render_particles(device, pass, particles, particleRender, nParticles, view, projection);
    if (this->showEField)    render_fields(device, pass, eFieldRender, fields.eField, cells.size(), view, projection);
    if (this->showBField)    render_fields(device, pass, bFieldRender, fields.bField, cells.size(), view, projection);
}

void Scene::compute_step(wgpu::Device& device, wgpu::ComputePassEncoder pass) {
    // Update current segments
    if (this->refreshCurrents) {
        this->cachedCurrents = get_currents();
        update_currents_buffer(device, this->currentSegmentsBuffer, this->cachedCurrents);
        this->refreshCurrents = false;
    }
    
    run_field_compute(
        device,
        pass,
        fieldCompute,
        static_cast<glm::u32>(cells.size()),
        static_cast<glm::u32>(cachedCurrents.size()),
        0.0f,
        enableParticleFieldContributions);

    run_particle_compute(
        device,
        pass,
        particleCompute,
        dt,
        0.0f,
        enableParticleFieldContributions,
        static_cast<glm::u32>(cachedCurrents.size()),
        nParticles);

    t += dt;
}

void Scene::compute_copy(wgpu::CommandEncoder& encoder) {
    encoder.CopyBufferToBuffer(particles.nCur, 0, particleCompute.nCurReadBuf, 0, sizeof(glm::u32));
}

void Scene::compute_read(wgpu::Device& device, wgpu::Instance& instance) {
    nParticles = read_nparticles(device, instance, particleCompute);
}

std::vector<Cell> Scene::get_grid_cells(glm::f32 spacing) {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return get_free_space_grid_cells(minCoord, maxCoord, 0.01f * _M);
}

glm::f32vec4 Scene::rand_particle_position() {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return free_space_rand_particle_position(minCoord, maxCoord);
}

std::vector<CurrentVector> Scene::get_currents() {
    std::vector<CurrentVector> empty;
    return empty;
}

glm::u32 Scene::getNumParticles() {
    return this->nParticles;
}

bool Scene::process_input(GLFWwindow* window, bool (*debounce_input)()) {
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
    return false;
}

void Scene::toggleShowAxes() {
    this->showAxes = !this->showAxes;
}

void Scene::toggleShowParticles() {
    this->showParticles = !this->showParticles;
}

void Scene::toggleShowEField() {
    this->showEField = !this->showEField;
}

void Scene::toggleShowBField() {
    this->showBField = !this->showBField;
}

void Scene::toggleShowETracers() {
    this->showETracers = !this->showETracers;
}

void Scene::toggleShowBTracers() {
    this->showBTracers = !this->showBTracers;
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

glm::u32 Scene::getNumTracers() {
    return this->nTracers;
}

glm::u32 Scene::getTracerPoints() {
    return this->tracerPoints;
}