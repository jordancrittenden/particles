#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "field_vector.h"

#define ARROW_TIP_SEGMENTS 10

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

// Vector arrow geometry - points along the z-axis
std::vector<float> create_vector_geometry(float length) {
    std::vector<float> vertices;

    float tipWidth = length / 15.0f;
    float tipLength = length / 3.0f;
    // Vertices for the arrow shaft
    vertices.insert(vertices.end(), {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, length
    });
    // Vertices for the arrow tip
    vertices.insert(vertices.end(), {
         0.0f, 0.0f, length,
    });
    for (int i = 0; i <= ARROW_TIP_SEGMENTS; i++) {
        float angle = i * (2 * M_PI) / (float)ARROW_TIP_SEGMENTS;
        vertices.insert(vertices.end(), {
            tipWidth * cos(angle), tipWidth * sin(angle), length - tipLength
        });
    }

    return vertices;
}

// Update direction buffer data
void update_vectors_buffer(FieldGLBuffers& fieldBuf, const std::vector<glm::vec4>& vec) {
    glBindBuffer(GL_ARRAY_BUFFER, fieldBuf.instanceVecBuf);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec4) * vec.size(), vec.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FieldGLBuffers create_vectors_buffers(std::vector<glm::vec4>& loc, std::vector<glm::vec4>& vec, float length) {
    FieldGLBuffers field;

    std::vector<float> vertices = create_vector_geometry(length);

    // Instance buffers
    glGenBuffers(1, &field.instanceLocBuf);
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceLocBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * loc.size(), loc.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &field.instanceVecBuf);
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceVecBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vec.size(), vec.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Model buffer
    glGenVertexArrays(1, &field.arrowBuf.vao);
    glGenBuffers(1, &field.arrowBuf.vbo);
    glBindVertexArray(field.arrowBuf.vao);

    glBindBuffer(GL_ARRAY_BUFFER, field.arrowBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceLocBuf); // this attribute comes from a different vertex buffer
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, field.instanceVecBuf); // this attribute comes from a different vertex buffer
    glEnableVertexAttribArray(2); 
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    return field;
}

void render_fields(GLuint shader, int numFieldVectors, const FieldGLBuffers& eFieldBuf, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(eFieldBuf.arrowBuf.vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, numFieldVectors);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 2, ARROW_TIP_SEGMENTS + 2, numFieldVectors);
}