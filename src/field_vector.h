#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "gl_util.h"

typedef struct FieldGLBuffers {
    GLBuffers vectorBuf;
    GLuint instanceTranslationBuf;
    GLuint instanceRotationBuf;
} FieldGLBuffers;

FieldGLBuffers create_vectors_buffers(std::vector<glm::mat4>& translations, std::vector<glm::mat4>& rotations, float length);
void update_vectors_buffer(FieldGLBuffers& eFieldBuf, const std::vector<glm::mat4>& rotations);
void render_fields(GLuint shader, int numFieldVectors, const FieldGLBuffers& eFieldBuf, glm::mat4 view, glm::mat4 projection);