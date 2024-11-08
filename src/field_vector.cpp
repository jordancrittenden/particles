#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_util.h"

// Vector arrow geometry - points along the z-axis
void create_vector_geometry(std::vector<float>& vertices, float length) {
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, length});
    float tipWidth = length / 20.0f;
    float tipLenth = length / 10.0f;
    vertices.insert(vertices.end(), {
         0.0f,     0.0f,     length,
         tipWidth, tipWidth, length - tipLenth,
        -tipWidth, tipWidth, length - tipLenth,
         0.0f,    -tipWidth, length - tipLenth,
         tipWidth, tipWidth, length - tipLenth,
    });
}

// Initialize directions
std::vector<glm::vec3> random_vectors(int nVectors) {
    std::vector<glm::vec3> directions;
    for (int i = 0; i < nVectors; ++i) {
        directions.push_back(glm::normalize(glm::vec3(
            static_cast<float>(rand() % 10 - 5) / 10.0f,
            static_cast<float>(rand() % 10 - 5) / 10.0f,
            static_cast<float>(rand() % 10 - 5) / 10.0f
        )));
    }
    return directions;
}

// Update direction buffer data
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::vec3>& directions) {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * directions.size(), sizeof(glm::vec3), directions.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLBuffers create_vectors_buffers(std::vector<glm::vec3>& directions, float length) {
    std::vector<float> vertices;
    create_vector_geometry(vertices, length);

    std::vector<float> instanceData;
    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < directions.size(); ++i) {
        instanceData.push_back(static_cast<float>(rand() % 20 - 10) / 10.0f);
        instanceData.push_back(static_cast<float>(rand() % 20 - 10) / 10.0f);
        instanceData.push_back(static_cast<float>(rand() % 20 - 10) / 10.0f);
        instanceData.push_back(directions[i].x);
        instanceData.push_back(directions[i].y);
        instanceData.push_back(directions[i].z);
        instanceData.push_back(static_cast<float>(rand()) / RAND_MAX);
        instanceData.push_back(static_cast<float>(rand()) / RAND_MAX);
        instanceData.push_back(static_cast<float>(rand()) / RAND_MAX);
    }

    GLBuffers buf;
    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);
    glGenBuffers(1, &buf.instance_vbo);

    glBindVertexArray(buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, buf.instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(0));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);

    return buf;
}