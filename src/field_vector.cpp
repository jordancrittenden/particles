#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_util.h"

#define PI   (3.14159265953f)

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

// Vector arrow geometry - points along the z-axis
std::vector<float> create_vector_geometry(float length) {
    std::vector<float> vertices;

    float tipWidth = length / 20.0f;
    float tipLenth = length / 10.0f;
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

// Initialize directions
std::vector<glm::mat4> random_transforms(int nVectors) {
    std::vector<glm::mat4> transforms;
    for (int i = 0; i < nVectors; ++i) {
        glm::mat4 xform = glm::mat4(1.0f);
        xform = glm::rotate(xform, glm::radians(rand_range(0.0f, 2 * PI)), glm::vec3(rand_range(0.0f, 1.0f), rand_range(0.0f, 1.0f), rand_range(0.0f, 1.0f)));
        xform = glm::translate(xform, glm::vec3(rand_range(0.0f, 1.0f), rand_range(0.0f, 1.0f), rand_range(0.0f, 1.0f)));
        transforms.push_back(xform);
    }
    return transforms;
}

// Update direction buffer data
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::mat4>& transforms) {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
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