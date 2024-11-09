#pragma once

#include <glm/glm.hpp>

GLBuffers create_axes_buffers();
void render_axes(GLuint shader, GLBuffers axesBuf, glm::mat4 view, glm::mat4 projection);