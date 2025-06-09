#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../gl_util.h"

typedef struct FieldGLBuffers {
    GLBuffers arrowBuf;
    GLuint instanceLocBuf;
    GLuint instanceVecBuf;
} FieldGLBuffers;

FieldGLBuffers create_vectors_buffers(std::vector<glm::vec4>& loc, std::vector<glm::vec4>& vec, float length);
void update_vectors_buffer(FieldGLBuffers& eFieldBuf, const std::vector<glm::vec4>& vec);
void render_fields(GLuint shader, int numFieldVectors, const FieldGLBuffers& eFieldBuf, glm::mat4 view, glm::mat4 projection);