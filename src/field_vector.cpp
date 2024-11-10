#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gl_util.h"

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
void update_vectors_buffer(GLuint vbo, const std::vector<glm::mat4>& transforms) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * transforms.size(), transforms.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLBuffers create_vectors_buffers(std::vector<glm::mat4>& transforms, float length) {
    GLBuffers buf;

    std::vector<float> vertices = create_vector_geometry(length);

    // Instance buffer
    glGenBuffers(1, &buf.instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buf.instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * transforms.size(), transforms.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Model buffer
    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);
    glBindVertexArray(buf.vao);

    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // need 4 vec4 attributes to store a mat4
    std::size_t vec4Size = sizeof(glm::vec4);
    glBindBuffer(GL_ARRAY_BUFFER, buf.instance_vbo); // this attribute comes from a different vertex buffer
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glEnableVertexAttribArray(2); 
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glEnableVertexAttribArray(3); 
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(4); 
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    return buf;
}

void render_fields(GLuint shader, int numFieldVectors, const GLBuffers& eFieldBuf, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(eFieldBuf.vao);
    glDrawArraysInstanced(GL_LINES, 0, 2, numFieldVectors);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 2, 5, numFieldVectors);
}