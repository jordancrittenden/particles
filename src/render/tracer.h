#pragma once

#include <glm/glm.hpp>
#include "../util/cl_util.h"
#include "../util/gl_util.h"

GLBuffers create_tracer_buffer(std::vector<glm::vec4>& loc, int tracerLength);

void render_tracers(GLuint shader, GLBuffers tracers, int nTracers, int tracerLength, cl_float3 color, glm::mat4 view, glm::mat4 projection);