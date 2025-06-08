#pragma once

#include "gl_util.h"

typedef struct SolenoidParameters {
    float r = 0.15f; // Radius of the central solenoid, m
    float flux = 0.3f;  // Central solenoid magnetic flux, V*s
} SolenoidParameters;

void render_solenoid(GLuint shader, const GLBuffers& ringBuf, const SolenoidParameters& parameters, glm::mat4 view, glm::mat4 projection);