#ifndef __FIELD_VECTOR_H__
#define __FIELD_VECTOR_H__

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"

std::vector<glm::vec3> random_vectors(int nVectors);
GLBuffers create_vectors_buffers(std::vector<glm::vec3>& directions, float length);
void update_vectors_buffer(GLuint instanceVBO, const std::vector<glm::vec3>& directions);

#endif