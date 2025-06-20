#include <iostream>
#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "physical_constants.h"
#include "particles.h"

ParticleBuffers create_particle_buffers(
    wgpu::Device& device,
    std::function<glm::f32vec4()> posF,
    std::function<glm::f32vec4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    glm::u32 initialParticles,
    glm::u32 maxParticles
) {
    ParticleBuffers buf = {};
    std::vector<glm::f32vec4> position_and_type;
    std::vector<glm::f32vec4> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < maxParticles; ++i) {
        if (i < initialParticles) {
            PARTICLE_SPECIES species = speciesF();
            glm::f32vec4 pos = posF();
            glm::f32vec4 vel = velF(species);

            pos[3] = (float)species;

            // [x, y, z, species]
            position_and_type.push_back(pos);
            // [dx, dy, dz, unused]
            velocity.push_back(vel);
        } else {
            // Placeholders for future particles that may be created via collisions

            // [x, y, z, species]
            position_and_type.push_back(glm::f32vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
            // [dx, dy, dz, unused]
            velocity.push_back(glm::f32vec4 { 0.0f, 0.0f, 0.0f, 0.0f });
        }
    }

    // Create shared position buffer (used by both compute and render)
    wgpu::BufferDescriptor posDesc = {
        .label = "Shared Particle Position Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex,
        .mappedAtCreation = false
    };
    buf.pos = device.CreateBuffer(&posDesc);
    device.GetQueue().WriteBuffer(buf.pos, 0, position_and_type.data(), position_and_type.size() * sizeof(glm::f32vec4));

    // Create shared velocity buffer (used by both compute and render)
    wgpu::BufferDescriptor velDesc = {
        .label = "Shared Particle Velocity Buffer",
        .size = maxParticles * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage,
        .mappedAtCreation = false
    };
    buf.vel = device.CreateBuffer(&velDesc);
    device.GetQueue().WriteBuffer(buf.vel, 0, velocity.data(), velocity.size() * sizeof(glm::f32vec4));

    return buf;
}