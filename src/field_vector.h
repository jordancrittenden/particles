#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"

std::vector<glm::mat4> random_transforms(int nVectors);
GLBuffers create_vectors_buffers(std::vector<glm::mat4>& transforms, float length);
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::mat4>& transforms);
void render_fields(GLuint shader, int numFieldVectors, GLBuffers eFieldBuf, glm::mat4 view, glm::mat4 projection);