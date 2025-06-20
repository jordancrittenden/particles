#pragma once

#include <glm/glm.hpp>
#include "util/gl_util.h"

GLBuffers create_axes_buffers();

void render_axes(GLuint shader, const GLBuffers& axesBuf, glm::mat4 view, glm::mat4 projection);