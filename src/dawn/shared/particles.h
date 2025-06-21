#pragma once

#include <dawn/webgpu_cpp.h>
#include <functional>
#include <glm/glm.hpp>
#include "physical_constants.h"

struct ParticleBuffers {
    wgpu::Buffer nCur;   // Current number of particles
    wgpu::Buffer pos;    // Particle positions
    wgpu::Buffer vel;    // Particle velocities
    glm::u32 nMax;       // Maximum number of particles
};

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::f32vec4()> posF,
    std::function<glm::f32vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    glm::u32 initialParticles,
    glm::u32 maxParticles);