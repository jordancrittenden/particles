#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_util.h"
#include "cl_util.h"
#include "scene.h"
#include "state.h"
#include "geometry/axes.h"
#include "geometry/tracer.h"
#include "particles.h"
#include "current_segment.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

Scene::Scene(SimulationState& state) {
    this->state = &state;

    axesShaderProgram      = create_shader_program("shader/axes_vertex.glsl", "shader/axes_fragment.glsl");
    particlesShaderProgram = create_shader_program("shader/particles_vertex.glsl", "shader/particles_fragment.glsl");
    vectorShaderProgram    = create_shader_program("shader/vector_vertex.glsl", "shader/vector_fragment.glsl");
    tracerShaderProgram    = create_shader_program("shader/tracer_vertex.glsl", "shader/tracer_fragment.glsl");
}

void Scene::initialize() {
    state->cells = get_grid_cells(state->cellSpacing);
    std::cout << "Simulation cells: " << state->cells.size() << std::endl;

    this->axes = create_axes_buffers();
    
    create_particle_buffers(
        [this](){ return rand_particle_position(); },
        [this](PARTICLE_SPECIES species){ return maxwell_boltzmann_particle_velocty(state->initialTemperature, particle_mass(species)); },
        [](){ return rand_particle_species(0.0f, 0.3f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f); },
        this->pos,
        this->vel,
        state->initialParticles,
        state->maxParticles);
    
    std::vector<glm::vec4> eFieldLoc, eFieldVec;
    std::vector<glm::vec4> bFieldLoc, bFieldVec;
    for (auto& cell : state->cells) {
        eFieldLoc.push_back(glm::vec4(cell.pos.s[0], cell.pos.s[1], cell.pos.s[2], 0.0f)); // Last element indicates E vs B
        eFieldVec.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));  // initial (meaningless) value
        bFieldLoc.push_back(glm::vec4(cell.pos.s[0], cell.pos.s[1], cell.pos.s[2], 1.0f)); // Last element indicates E vs B
        bFieldVec.push_back(glm::vec4(-1.0f, 1.0f, 0.0f, 0.0f)); // initial (meaningless) value
    }

    this->e_field = create_vectors_buffers(eFieldLoc, eFieldVec, 0.03f * _M);
    this->b_field = create_vectors_buffers(bFieldLoc, bFieldVec, 0.03f * _M);

    std::vector<glm::vec4> tracerLoc;
    // Randomly select 1% of cells to use as initial tracer locations
    for (auto& cell : state->cells) {
        if (rand_range(0.0f, 1.0f) < 0.01f) {
            tracerLoc.push_back(glm::vec4(cell.pos.s[0], cell.pos.s[1], cell.pos.s[2], 0.0f));
        }
    }
    this->tracers = create_tracer_buffer(tracerLoc, this->tracerPoints);
    this->nTracers = tracerLoc.size();

    this->cameraDistance = 0.5f * _M;
}

Scene::~Scene() {
    glDeleteVertexArrays(1, &this->axes.vao);
    glDeleteVertexArrays(1, &this->pos.vao);
    glDeleteVertexArrays(1, &this->e_field.arrowBuf.vao);
    glDeleteVertexArrays(1, &this->b_field.arrowBuf.vao);
    glDeleteVertexArrays(1, &this->tracers.vao);
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

void Scene::render(float aspectRatio) {
    this->view = get_orbit_view_matrix();
    this->projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    if (this->showAxes)      render_axes(axesShaderProgram, this->axes, view, projection);
    if (this->showParticles) render_particles(particlesShaderProgram, this->pos, /*nParticles*/ state->maxParticles, view, projection);
    if (this->showEField)    render_fields(vectorShaderProgram, state->cells.size(), this->e_field, view, projection);
    if (this->showBField)    render_fields(vectorShaderProgram, state->cells.size(), this->b_field, view, projection);
    if (this->showTracers)   render_tracers(tracerShaderProgram, this->tracers, this->nTracers, this->tracerPoints, view, projection);
}

std::vector<Cell> Scene::get_grid_cells(float spacing) {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return get_free_space_grid_cells(minCoord, maxCoord, 0.01f * _M);
}

cl_float4 Scene::rand_particle_position() {
    float s = 0.1f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    return free_space_rand_particle_position(minCoord, maxCoord);
}

std::vector<CurrentVector> Scene::get_currents() {
    std::vector<CurrentVector> empty;
    return empty;
}

cl::BufferGL Scene::getParticlePosBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->pos.vbo, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

cl::BufferGL Scene::getParticleVelBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->vel.vbo, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

cl::BufferGL Scene::getEFieldVecBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->e_field.instanceVecBuf, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

cl::BufferGL Scene::getBFieldVecBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->b_field.instanceVecBuf, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

cl::BufferGL Scene::getTracerBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->tracers.vbo, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
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

void Scene::toggleShowTracers() {
    this->showTracers = !this->showTracers;
}

void Scene::zoomIn() {
    this->cameraDistance /= 1.01f;
}

void Scene::zoomOut() {
    this->cameraDistance *= 1.01f;
}

void Scene::rotateLeft() {
    this->cameraPhi -= 0.01f;
}

void Scene::rotateRight() {
    this->cameraPhi += 0.01f;
}

void Scene::rotateUp() {
    this->cameraTheta -= 0.01f;
}

void Scene::rotateDown() {
    this->cameraTheta += 0.01f;
}

int Scene::getNumTracers() {
    return this->nTracers;
}

int Scene::getTracerPoints() {
    return this->tracerPoints;
}