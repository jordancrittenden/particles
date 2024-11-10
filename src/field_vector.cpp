#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "field_vector.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

// Vector arrow geometry - points along the z-axis
std::vector<float> create_vector_geometry(float length) {
    std::vector<float> vertices;

    float tipWidth = length / 6.0f;
    float tipLenth = length / 3.0f;
    // Vertices for the arrow shaft
    vertices.insert(vertices.end(), {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, length
    });
    // Vertices for the arrow tip
    vertices.insert(vertices.end(), {
         0.0f,     0.0f,     length,
         tipWidth, tipWidth, length - tipLenth,
        -tipWidth, tipWidth, length - tipLenth,
         0.0f,    -tipWidth, length - tipLenth,
         tipWidth, tipWidth, length - tipLenth,
    });

    return vertices;
}

// Update direction buffer data
void update_vectors_buffer(FieldGLBuffers& eFieldBuf, const std::vector<glm::mat4>& rotations) {
    glBindBuffer(GL_ARRAY_BUFFER, eFieldBuf.instanceRotationBuf);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * rotations.size(), rotations.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FieldGLBuffers create_vectors_buffers(std::vector<glm::mat4>& translations, std::vector<glm::mat4>& rotations, float length) {
    FieldGLBuffers field;

    std::vector<float> vertices = create_vector_geometry(length);

    // Instance buffers
    glGenBuffers(1, &field.instanceTranslationBuf);
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceTranslationBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * translations.size(), translations.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &field.instanceRotationBuf);
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceRotationBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * rotations.size(), rotations.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Model buffer
    glGenVertexArrays(1, &field.vectorBuf.vao);
    glGenBuffers(1, &field.vectorBuf.vbo);
    glBindVertexArray(field.vectorBuf.vao);

    glBindBuffer(GL_ARRAY_BUFFER, field.vectorBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // need 4 vec4 attributes to store a mat4
    std::size_t vec4Size = sizeof(glm::vec4);
    glBindBuffer(GL_ARRAY_BUFFER, field.instanceTranslationBuf); // this attribute comes from a different vertex buffer
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glEnableVertexAttribArray(2); 
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glEnableVertexAttribArray(3); 
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(4); 
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, field.instanceRotationBuf); // this attribute comes from a different vertex buffer
    glEnableVertexAttribArray(5); 
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glEnableVertexAttribArray(6); 
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glEnableVertexAttribArray(7); 
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(8); 
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);

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

    glBindVertexArray(eFieldBuf.vectorBuf.vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, numFieldVectors);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 2, 5, numFieldVectors);
}