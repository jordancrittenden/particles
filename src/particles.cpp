#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gl_util.h"
#include "cl_util.h"

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

// Create buffers for particle position/velocity
void create_particle_buffers(GLBuffers& posBuf, GLBuffers& velBuf, int nParticles) {
    std::vector<float> position_and_type;
    std::vector<float> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < nParticles; ++i) {
        float chargeRand = static_cast<float>(rand()) / RAND_MAX;
        //float charge = chargeRand < 0.33 ? 0.0 : (chargeRand < 0.66 ? 1.0 : -1.0);
        float charge = chargeRand < 0.5 ? 1.0 : -1.0;

        float r = rand_range(0.95f, 1.05f);
        float theta = rand_range(0.0f, 2 * M_PI);
        float y = rand_range(-0.05f, 0.05f);

        // [x, y, z, type]
        position_and_type.push_back(r * sin(theta));
        position_and_type.push_back(y);
        position_and_type.push_back(r * cos(theta));
        position_and_type.push_back(charge);

        // [dx, dy, dz, unused]
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(static_cast<float>(rand()) / RAND_MAX * 10.0f - 5.0f);
        velocity.push_back(0.0f);
    }

    // position/type buffer
    glGenVertexArrays(1, &posBuf.vao);
    glBindVertexArray(posBuf.vao);
    glGenBuffers(1, &posBuf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(cl_float4), position_and_type.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)0); // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)(3 * sizeof(float))); // charge attribute
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // velocity buffer
    glGenVertexArrays(1, &velBuf.vao);
    glBindVertexArray(velBuf.vao);
    glGenBuffers(1, &velBuf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(cl_float4), velocity.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw particles
    glBindVertexArray(posBuf.vao);
    glDrawArrays(GL_POINTS, 0, nParticles);
}