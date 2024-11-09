#pragma once

#include <glm/glm.hpp>
#include "gl_util.h"

void create_particle_buffers(GLBuffers& posBuf, GLBuffers& velBuf, int nParticles);
void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection);