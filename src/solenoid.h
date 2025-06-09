#pragma once

#include "physical_constants.h"
#include "gl_util.h"

typedef struct SolenoidParameters {
    float r = 0.15f * _M;         // Radius of the central solenoid, m
    float flux = 0.3f * _V * _S;  // Central solenoid magnetic flux, V s
} SolenoidParameters;

void render_solenoid(GLuint shader, const GLBuffers& ringBuf, float flux, glm::mat4 view, glm::mat4 projection);