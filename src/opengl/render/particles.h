#pragma once

#include <glm/glm.hpp>
#include "util/gl_util.h"

void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection);