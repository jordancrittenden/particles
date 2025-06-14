#include <vector>
#include <cmath>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/cl_util.h"
#include "util/gl_util.h"
#include "state.h"
#include "solenoid.h"

void render_solenoid(GLuint shader, const GLBuffers& solenoidBuf, float flux, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));

    // Set view and projection uniforms
    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    GLint fluxLoc = glGetUniformLocation(shader, "flux");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(fluxLoc, flux);

    glBindVertexArray(solenoidBuf.vao);
    glDrawElements(GL_TRIANGLES, solenoidBuf.indices.size(), GL_UNSIGNED_INT, 0);
}