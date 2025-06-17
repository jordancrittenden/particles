#pragma once

#include <vector>

// A ring centered on the z-axis with an inner radius r, radial thickness t, and z-depth d
typedef struct Ring {
    float r = 1.0f; // Radius of the ring, m
    float t = 1.0f; // Thickness of the ring, m
    float d = 1.0f; // Depth of the ring, m
} Ring;

void generate_ring_vertices(const Ring& ring, std::vector<float>& vertices, std::vector<unsigned int>& indices);
