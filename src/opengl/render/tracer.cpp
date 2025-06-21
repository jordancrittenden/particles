#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "physical_constants.h"
#include "util/gl_util.h"
#include "util/cl_util.h"

GLBuffers create_tracer_buffer(std::vector<glm::f32vec4>& loc, int tracerPoints) {
    std::vector<cl_float3> tracerTrails;

    float sep = 0.001f * _M;

    // Trace geometry - points along the x-axis
    for (auto& pos : loc) {
        for (int i = 0; i <= tracerPoints; i++) {
            tracerTrails.push_back(cl_float4 { pos.x + i * sep, pos.y, pos.z });
        }
    }

    GLBuffers buf;
    glGenVertexArrays(1, &buf.vao);
    glGenBuffers(1, &buf.vbo);

    glBindVertexArray(buf.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf.vbo);
    glBufferData(GL_ARRAY_BUFFER, loc.size() * tracerPoints * sizeof(cl_float3), tracerTrails.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return buf;
}

void render_tracers(GLuint shader, GLBuffers tracersBuf, int nTracers, int tracerLength, cl_float3 color, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");
    GLuint colorLoc = glGetUniformLocation(shader, "in_color");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorLoc, color.s[0], color.s[1], color.s[2]);

    // Draw tracers
    glBindVertexArray(tracersBuf.vao);
    glDrawArrays(GL_POINTS, 0, nTracers * tracerLength);
}