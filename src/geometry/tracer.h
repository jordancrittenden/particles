#pragma once

#include <glm/glm.hpp>
#include "../gl_util.h"
#include "../cl_util.h"

GLBuffers create_tracer_buffer(std::vector<glm::vec4>& loc, int tracerLength);
void render_tracers(GLuint shader, GLBuffers tracers, int nTracers, int tracerLength, glm::mat4 view, glm::mat4 projection);