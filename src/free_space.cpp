#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include "free_space.h"
#include "emscripten_key.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

FreeSpaceScene::FreeSpaceScene() : Scene() {
}

void FreeSpaceScene::init(const SimulationParams& params) {
    Scene::init(params);

    this->boundaryCompute = create_boundary_compute(device, particles, params.maxParticles);
    this->cameraDistance = 3.0f * _M;
}

void FreeSpaceScene::render_details(wgpu::RenderPassEncoder& pass) {
    Scene::render_details(pass);
}

void FreeSpaceScene::compute_field_step(wgpu::ComputePassEncoder& pass) {
    run_field_compute(
        device,
        pass,
        fieldCompute,
        static_cast<glm::u32>(cells.size()),
        static_cast<glm::u32>(cachedCurrents.size()),
        0.0f, // loop voltage
        0.0f, // solenoid radius
        enableParticleFieldContributions);

    // Run tracer compute
    run_tracer_compute(
        device,
        pass,
        tracerCompute,
        dt,
        0.0f, // loop voltage
        0.0f, // solenoid radius
        enableParticleFieldContributions,
        static_cast<glm::u32>(cachedCurrents.size()),
        nParticles,
        tracers.nTracers,
        TRACER_LENGTH);
}

void FreeSpaceScene::compute_wall_interactions(wgpu::ComputePassEncoder& pass) {
    run_boundary_compute(
        device,
        pass,
        boundaryCompute,
        mesh.min.x,
        mesh.max.x,
        mesh.min.y,
        mesh.max.y,
        mesh.min.z,
        mesh.max.z,
        nParticles);
}

std::vector<Cell> FreeSpaceScene::get_mesh_cells(glm::f32vec3 size, MeshProperties& mesh) {
    float s = 1.0f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };
    
    std::vector<Cell> cells;

    // Use integer counts so float accumulation can't drop the last row of nodes
    glm::u32vec3 dim = glm::u32vec3(glm::round((maxCoord - minCoord) / size)) + 1u;

    for (glm::u32 ix = 0; ix < dim.x; ix++) {
        float x = minCoord.x + ix * size.x;
        for (glm::u32 iz = 0; iz < dim.z; iz++) {
            float z = minCoord.z + iz * size.z;
            for (glm::u32 iy = 0; iy < dim.y; iy++) {
                float y = minCoord.y + iy * size.y;
                Cell cell;
                cell.pos = glm::f32vec4 { x, y, z, 1.0f };
                cell.min = glm::f32vec3 { x - size.x/2.0f, y - size.y/2.0f, z - size.z/2.0f };
                cell.max = glm::f32vec3 { x + size.x/2.0f, y + size.y/2.0f, z + size.z/2.0f };
                cells.push_back(cell);
            }
        }
    }
    mesh.dim = dim;
    mesh.cell_size = size;
    mesh.min = minCoord;
    mesh.max = minCoord + glm::f32vec3(dim - 1u) * size; // last cell center actually generated
    
    return cells;
}

glm::f32vec4 FreeSpaceScene::rand_particle_position() {
    float s = 1.0f * _M;
    glm::vec3 minCoord { -s, -s, -s };
    glm::vec3 maxCoord { s, s, s };

    float x = rand_range(minCoord.x, maxCoord.x);
    float y = rand_range(minCoord.y, maxCoord.y);
    float z = rand_range(minCoord.z, maxCoord.z);

    // [x, y, z, unused]
    return glm::f32vec4 { x, y, z, 0.0f };
}

std::vector<CurrentVector> FreeSpaceScene::get_currents() {
    std::vector<CurrentVector> currents;
    // Dummy current (compute shader fails if currents is empty)
    currents.push_back(CurrentVector {
        .x = glm::f32vec4(0.0f, 0.0f, 0.0f, 0.0f),
        .dx = glm::f32vec4(1.0f, 0.0f, 0.0f, 0.0f),
        .i = 0.0f
    });
    return currents;
}

bool FreeSpaceScene::process_input(bool (*debounce_input)()) {
#if defined(__EMSCRIPTEN__)
    
#else
    
#endif
    return Scene::process_input(debounce_input);
}