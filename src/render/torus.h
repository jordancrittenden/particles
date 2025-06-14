#pragma once

#include <glm/glm.hpp>
#include "../util/gl_util.h"

glm::mat4 get_coil_model_matrix(float angle, float r1);

void render_torus_rings(GLuint shader, const GLBuffers& torusRingBug, int nCoils, float primaryRadius, float current, glm::mat4 view, glm::mat4 projection);