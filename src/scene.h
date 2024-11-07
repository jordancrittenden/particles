#ifndef __SCENE_H__
#define __SCENE_H__

#include "gl_util.h"

typedef struct Scene {
    // Torus geometry and buffers
    GLBufPair torus;

    // Particle state buffers
    GLBufPair pos; // pos.vbo: [x0, y0, z0, charge0, x1, y1, z1, charge1, ...]
    GLBufPair vel; // vel.vbo: [x0, y0, z0, unused, x1, y1, z1, unused, ...]

    // X,Y,Z axes buffers
    GLBufPair axes;

    // Camera settings
    float rotAngle = 0.0f;
    float cameraDistance = 5.0f;

    // Show/hide booleans
    bool showAxes = true;
    bool showTorus = true;
    bool showParticles = true;
} Scene;

#endif