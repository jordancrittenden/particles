#include <vector>
#include <cmath>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../gl_util.h"
#include "ring.h"

void generate_ring_vertices(const Ring& ring, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    int radialSegments = 64;
    float outerRadius = ring.r + ring.t;

    // Generate vertices for back face
    float z = -ring.d / 2.0f;
    for (int i = 0; i < radialSegments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / radialSegments;
        float xInner = ring.r * cos(angle);
        float yInner = ring.r * sin(angle);
        float xOuter = outerRadius * cos(angle);
        float yOuter = outerRadius * sin(angle);

        // Inner vertex
        vertices.insert(vertices.end(), { xInner, yInner, z, 0.0f, 0.0f, -1.0f });
        // Outer vertex
        vertices.insert(vertices.end(), { xOuter, yOuter, z, 0.0f, 0.0f, -1.0f });
    }

    // Generate indices for back face of the ring
    for (int i = 0; i < radialSegments; ++i) {
        unsigned int idx = i * 2;
        unsigned int nextIdx = (i < radialSegments - 1) ? idx + 2 : 0;

        // Front and back faces
        indices.insert(indices.end(), { idx, idx + 1, nextIdx + 1 });
        indices.insert(indices.end(), { idx, nextIdx + 1, nextIdx });
    }

    // Generate vertices for front face
    z = ring.d / 2.0f;
    unsigned int frontFaceStartIdx = vertices.size() / 6;
    for (int i = 0; i < radialSegments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / radialSegments;
        float xInner = ring.r * cos(angle);
        float yInner = ring.r * sin(angle);
        float xOuter = outerRadius * cos(angle);
        float yOuter = outerRadius * sin(angle);

        // Inner vertex
        vertices.insert(vertices.end(), { xInner, yInner, z, 0.0f, 0.0f, 1.0f });
        // Outer vertex
        vertices.insert(vertices.end(), { xOuter, yOuter, z, 0.0f, 0.0f, 1.0f });
    }

    // Generate indices for front face of the ring
    for (int i = 0; i < radialSegments; ++i) {
        unsigned int idx = frontFaceStartIdx + i * 2;
        unsigned int nextIdx = (i < radialSegments - 1) ? idx + 2 : frontFaceStartIdx;

        // Front and back faces
        indices.insert(indices.end(), { idx, idx + 1, nextIdx + 1 });
        indices.insert(indices.end(), { idx, nextIdx + 1, nextIdx });
    }

    // Generate vertices for inner rim of the ring
    unsigned int innerRimStartIdx = vertices.size() / 6;
    for (int i = 0; i < radialSegments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / radialSegments;
        float xInner = ring.r * cos(angle);
        float yInner = ring.r * sin(angle);

        // Back vertex
        vertices.insert(vertices.end(), { xInner, yInner, -ring.d/2.0f, -cos(angle), -sin(angle), 0.0f });
        // Front vertex
        vertices.insert(vertices.end(), { xInner, yInner, ring.d/2.0f, -cos(angle), -sin(angle), 0.0f });
    }

    // Generate inner rim indices
    for (int i = 0; i < radialSegments; ++i) {
        unsigned int idx = innerRimStartIdx + i * 2;
        unsigned int nextIdx = (i < radialSegments - 1) ? idx + 2 : innerRimStartIdx;

        // Connecting outer radius vertices on front and back faces
        indices.insert(indices.end(), { idx, idx + 1, nextIdx + 1 });
        indices.insert(indices.end(), { idx, nextIdx + 1, nextIdx });
    }

    // Generate vertices for outer rim of the ring
    unsigned int outerRimStartIdx = vertices.size() / 6;
    for (int i = 0; i < radialSegments; ++i) {
        float angle = i * 2.0f * glm::pi<float>() / radialSegments;
        float xOuter = outerRadius * cos(angle);
        float yOuter = outerRadius * sin(angle);

        // Back vertex
        vertices.insert(vertices.end(), { xOuter, yOuter, -ring.d/2.0f, cos(angle), sin(angle), 0.0f });
        // Front vertex
        vertices.insert(vertices.end(), { xOuter, yOuter, ring.d/2.0f, cos(angle), sin(angle), 0.0f });
    }

    // Generate outer rim indices
    for (int i = 0; i < radialSegments; ++i) {
        unsigned int idx = outerRimStartIdx + i * 2;
        unsigned int nextIdx = (i < radialSegments - 1) ? idx + 2 : outerRimStartIdx;

        // Connecting outer radius vertices on front and back faces
        indices.insert(indices.end(), { idx, idx + 1, nextIdx + 1 });
        indices.insert(indices.end(), { idx, nextIdx + 1, nextIdx });
    }
}

GLBuffers create_ring_buffers(const Ring& ring) {
    GLBuffers buf;

    // Generate ring vertices and indices
    std::vector<float> vertices;
    generate_ring_vertices(ring, vertices, buf.indices);

    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);
    glGenBuffers(1, &buf.ebo);

    glBindVertexArray(buf.vao);

    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, buf.indices.size() * sizeof(unsigned int), buf.indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return buf;
}