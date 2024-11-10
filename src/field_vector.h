#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"

GLBuffers create_vectors_buffers(std::vector<glm::mat4>& transforms, float length);
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::mat4>& transforms);
void render_fields(GLuint shader, int numFieldVectors, const GLBuffers& eFieldBuf, glm::mat4 view, glm::mat4 projection);