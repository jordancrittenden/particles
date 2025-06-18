#pragma once

#include <glm/glm.hpp>
#include <vector>

// A ring centered on the z-axis with an inner radius r, radial thickness t, and z-depth d
typedef struct Ring {
    glm::f32 r = 1.0f; // Radius of the ring, m
    glm::f32 t = 1.0f; // Thickness of the ring, m
    glm::f32 d = 1.0f; // Depth of the ring, m
} Ring;

void generate_ring_vertices(const Ring& ring, std::vector<glm::f32>& vertices, std::vector<glm::u32>& indices);
