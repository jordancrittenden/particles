#ifndef __FIELD_VECTOR_H__
#define __FIELD_VECTOR_H__

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"

std::vector<glm::mat4> random_transforms(int nVectors);
GLBuffers create_vectors_buffers(std::vector<glm::mat4>& transforms, float length);
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::mat4>& transforms);

#endif