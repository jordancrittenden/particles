#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "util/gl_util.h"
#include "util/cl_util.h"
#include "render/axes.h"
#include "render/tracer.h"
#include "render/particles.h"
#include "scene.h"
#include "state.h"
#include "plasma.h"
#include "current_segment.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

Scene::Scene(SimulationState& state) {
    this->state = &state;

    axesShaderProgram      = create_shader_program("shader/opengl/axes_vertex.glsl", "shader/opengl/axes_fragment.glsl");
    particlesShaderProgram = create_shader_program("shader/opengl/particles_vertex.glsl", "shader/opengl/particles_fragment.glsl");
    vectorShaderProgram    = create_shader_program("shader/opengl/vector_vertex.glsl", "shader/opengl/vector_fragment.glsl");
    tracerShaderProgram    = create_shader_program("shader/opengl/tracer_vertex.glsl", "shader/opengl/tracer_fragment.glsl");
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
    int i = 0;
    for (auto& cell : state->cells) {
        if (i % 10 == 0) {
            tracerLoc.push_back(glm::vec4(cell.pos.s[0], cell.pos.s[1], cell.pos.s[2], 0.0f));
        }
        i++;
    }
    this->e_tracers = create_tracer_buffer(tracerLoc, this->tracerPoints);
    this->b_tracers = create_tracer_buffer(tracerLoc, this->tracerPoints);
    this->nTracers = tracerLoc.size();

    this->cameraDistance = 0.5f * _M;
}

Scene::~Scene() {
    glDeleteVertexArrays(1, &this->axes.vao);
    glDeleteVertexArrays(1, &this->pos.vao);
    glDeleteVertexArrays(1, &this->e_field.arrowBuf.vao);
    glDeleteVertexArrays(1, &this->b_field.arrowBuf.vao);
    glDeleteVertexArrays(1, &this->e_tracers.vao);
    glDeleteVertexArrays(1, &this->b_tracers.vao);
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
    if (this->showETracers)  render_tracers(tracerShaderProgram, this->e_tracers, this->nTracers, this->tracerPoints, cl_float3{1.0f, 0.0f, 0.0f}, view, projection);
    if (this->showBTracers)  render_tracers(tracerShaderProgram, this->b_tracers, this->nTracers, this->tracerPoints, cl_float3{0.0f, 0.0f, 1.0f}, view, projection);
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

cl::BufferGL Scene::getETracerBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->e_tracers.vbo, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

cl::BufferGL Scene::getBTracerBufCL(cl::Context* context) {
    cl_int err;
    cl::BufferGL buf = cl::BufferGL(*context, CL_MEM_READ_WRITE, this->b_tracers.vbo, &err);
    cl_exit_if_err(err, "Failed to create OpenCL buffer from OpenGL buffer");
    return buf;
}

bool Scene::process_input(GLFWwindow* window, bool (*debounce_input)()) {
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

int Scene::getNumTracers() {
    return this->nTracers;
}

int Scene::getTracerPoints() {
    return this->tracerPoints;
}