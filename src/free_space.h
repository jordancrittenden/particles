#pragma once

#include "util/wgpu_util.h"
#include "scene.h"
#include "args.h"


class FreeSpaceScene : public Scene {
public:
    FreeSpaceScene();

    // Initialization
    void init(const SimulationParams& params) override;

    // Rendering
    void render_details(wgpu::RenderPassEncoder& pass) override;

    // Compute
    void compute_field_step(wgpu::ComputePassEncoder& pass) override;
    void compute_wall_interactions(wgpu::ComputePassEncoder& pass) override;

    // Scene-dependent functions
    std::vector<Cell> get_mesh_cells(glm::f32vec3 size, MeshProperties& mesh) override;
    glm::f32vec4 rand_particle_position() override;
    std::vector<CurrentVector> get_currents() override;
    bool process_input(bool (*debounce_input)()) override;

private:
};