#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util/gl_util.h"
#include "physical_constants.h"

// Create buffers for axes (X, Y, Z)
GLBuffers create_axes_buffers() {
    float axisVertices[] = {
        // X-axis
        -10.0f * _M, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,
         10.0f * _M, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,
        // Y-axis
        0.0f, -10.0f * _M, 0.0f,   0.0f, 0.0f, 0.0f,
        0.0f,  10.0f * _M, 0.0f,   0.0f, 0.0f, 0.0f,
        // Z-axis
        0.0f, 0.0f, -10.0f * _M,   0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,  10.0f * _M,   0.0f, 0.0f, 0.0f
    };

    GLBuffers buf;
    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);

    glBindVertexArray(buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return buf;
}

void render_axes(GLuint shader, const GLBuffers& axesBuf, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw axes
    glBindVertexArray(axesBuf.vao);
    glDrawArrays(GL_LINES, 0, 6);
}