#include <vector>
#include <cmath>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/gl_util.h"
#include "state.h"
#include "torus.h"

// Set up transformation matrix for each circle
glm::mat4 get_coil_model_matrix(float angle, float r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

void render_torus(GLuint shader, const GLBuffers& torusBuf, const TorusParameters& parameters, float current, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    // Set view and projection uniforms
    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    GLint currentLoc = glGetUniformLocation(shader, "current");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(currentLoc, current);

    // Draw each circle in the torus
    for (int i = 0; i < parameters.toroidalCoils; ++i) {
        float angle = (2.0f * M_PI * i) / parameters.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, parameters.r1);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(torusBuf.vao);
        glDrawElements(GL_TRIANGLES, torusBuf.indices.size(), GL_UNSIGNED_INT, 0);
    }
}