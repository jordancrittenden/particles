#pragma once

#include <glm/glm.hpp>
#include "../util/gl_util.h"

void render_solenoid(GLuint shader, const GLBuffers& solenoidRingBuf, float flux, glm::mat4 view, glm::mat4 projection);