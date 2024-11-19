#pragma once

#include "gl_util.h"
#include "field_vector.h"

typedef struct Scene {
    // OpenGL window
    GLFWwindow* window = nullptr;
    int windowWidth = 1600;
    int windowHeight = 1200;

    // Torus geometry and buffers
    GLBuffers torus;

    // Particle state buffers
    GLBuffers pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBuffers vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBuffers axes;

    // Field vectors
    FieldGLBuffers e_field;
    FieldGLBuffers b_field;

    // Camera settings
    float cameraDistance = 5.0f;
    float cameraTheta = 5.0f/6.0f * M_PI_2;
    float cameraPhi = 1.0f/6.0f * M_PI_2;

    // FPS
    int targetFPS = 60;

    // Show/hide booleans
    bool showAxes = true;
    bool showTorus = true;
    bool showParticles = true;
    bool showEField = false;
    bool showBField = false;
} Scene;