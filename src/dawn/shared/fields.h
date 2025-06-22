#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>

struct FieldBuffers {
    wgpu::Buffer eField;    // Electric field
    wgpu::Buffer bField;    // Magnetic field
    glm::u32 nCells;        // Number of cells
};

FieldBuffers create_fields_buffers(wgpu::Device& device, glm::u32 nCells);