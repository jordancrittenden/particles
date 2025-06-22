#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include "fields.h"

FieldBuffers create_fields_buffers(wgpu::Device& device, glm::u32 nCells) {
    FieldBuffers fieldBuf = {.nCells = nCells};

    wgpu::BufferDescriptor eFieldDesc = {
        .label = "Electric Field Buffer",
        .size = nCells * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex
    };
    fieldBuf.eField = device.CreateBuffer(&eFieldDesc);

    wgpu::BufferDescriptor bFieldDesc = {
        .label = "Magnetic Field Buffer",
        .size = nCells * sizeof(glm::f32vec4),
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex
    };
    fieldBuf.bField = device.CreateBuffer(&bFieldDesc);

    return fieldBuf;
}