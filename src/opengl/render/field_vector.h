#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "util/gl_util.h"

typedef struct FieldGLBuffers {
    GLBuffers arrowBuf;
    GLuint instanceLocBuf;
    GLuint instanceVecBuf;
} FieldGLBuffers;

FieldGLBuffers create_vectors_buffers(std::vector<glm::f32vec4>& loc, std::vector<glm::f32vec4>& vec, float length);

void update_vectors_buffer(FieldGLBuffers& fieldBuf, const std::vector<glm::f32vec4>& vec);

void render_fields(GLuint shader, int numFieldVectors, const FieldGLBuffers& fieldBuf, glm::mat4 view, glm::mat4 projection);