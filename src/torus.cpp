#include <vector>
#include <cmath>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cl_util.h"
#include "gl_util.h"
#include "state.h"
#include "torus.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

// Set up transformation matrix for each circle
glm::mat4 get_coil_model_matrix(float angle, float r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

void generate_ring_vertices(TorusParameters& parameters, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    int radialSegments = 64;
    int depthSegments = 1;
    float t = 0.05;
    float d = 0.1;
    float outerRadius = parameters.r2 + t;

    // Generate vertices for inner and outer circles at both front and back faces
    for (int j = 0; j <= depthSegments; ++j) {
        float z = (j == 0) ? -d / 2.0f : d / 2.0f;

        for (int i = 0; i <= radialSegments; ++i) {
            float angle = i * 2.0f * glm::pi<float>() / radialSegments;
            float xInner = parameters.r2 * cos(angle);
            float yInner = parameters.r2 * sin(angle);
            float xOuter = outerRadius * cos(angle);
            float yOuter = outerRadius * sin(angle);

            // Inner vertex
            vertices.insert(vertices.end(), { xInner, yInner, z, 0.0f, 0.0f, (j == 0) ? -1.0f : 1.0f });
            // Outer vertex
            vertices.insert(vertices.end(), { xOuter, yOuter, z, 0.0f, 0.0f, (j == 0) ? -1.0f : 1.0f });
        }
    }

    // Generate indices for front and back faces of the ring
    for (int j = 0; j <= depthSegments; ++j) {
        for (int i = 0; i < radialSegments; ++i) {
            unsigned int idx = (radialSegments + 1) * 2 * j + i * 2;

            // Front and back faces
            indices.insert(indices.end(), { idx, idx + 1, idx + 3 });
            indices.insert(indices.end(), { idx, idx + 3, idx + 2 });
        }
    }

    // Generate indices for radial sides (connecting inner and outer radii around the ring)
    for (int i = 0; i < radialSegments; ++i) {
        unsigned int innerStart = i * 2;
        unsigned int outerStart = innerStart + (radialSegments + 1) * 2;

        // Connecting outer radius vertices on front and back faces
        indices.insert(indices.end(), { innerStart, outerStart, outerStart + 2 });
        indices.insert(indices.end(), { innerStart, outerStart + 2, innerStart + 2 });

        // Connecting inner radius vertices on front and back faces
        indices.insert(indices.end(), { innerStart + 1, outerStart + 1, outerStart + 3 });
        indices.insert(indices.end(), { innerStart + 1, outerStart + 3, innerStart + 3 });
    }
}

GLBuffers create_torus_buffers(TorusParameters& parameters) {
    GLBuffers buf;

    // Generate ring vertices and indices
    std::vector<float> vertices;
    generate_ring_vertices(parameters, vertices, buf.indices);

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

void render_torus(GLuint shader, const GLBuffers& torusBuf, TorusParameters& parameters, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(torusBuf.vao);

    // Draw each circle in the torus
    for (int i = 0; i < parameters.toroidalCoils; ++i) {
        float angle = (2.0f * M_PI * i) / parameters.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, parameters.r1);

        GLint modelLoc = glGetUniformLocation(shader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(torusBuf.vao);
        glDrawElements(GL_TRIANGLES, torusBuf.indices.size(), GL_UNSIGNED_INT, 0);
    }
}