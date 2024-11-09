#pragma once

#include "gl_util.h"

typedef struct Scene {
    // Torus geometry and buffers
    GLBuffers torus;

    // Particle state buffers
    GLBuffers pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBuffers vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBuffers axes;

    // Field vectors
    GLBuffers e_field;

    // Camera settings
    float rotAngle = 0.0f;
    float cameraDistance = 5.0f;

    // Show/hide booleans
    bool showAxes = true;
    bool showTorus = true;
    bool showParticles = true;
    bool showEField = true;
} Scene;