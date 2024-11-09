#include <glm/gtc/matrix_transform.hpp>
#include "torus.h"

#define MU_0 (1.25663706144e-6f) /* kg m / A^2 s^2 */

// Generate vertices for a circle with radius r2
std::vector<float> generate_coil_vertices_unrolled(float r2, int segments) {
    std::vector<float> vertices;
    float thetaStep = 2.0f * M_PI / segments;
    for (int i = 0; i < segments; ++i) {
        float theta = i * thetaStep;
        vertices.push_back(r2 * cos(theta));
        vertices.push_back(r2 * sin(theta));
    }
    return vertices;
}

// Set up transformation matrix for each circle
glm::mat4 get_coil_model_matrix(float angle, float r1) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(r1, 0.0f, 0.0f));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    return model;
}

std::vector<CurrentVector> get_toroidal_currents(TorusProperties& torus) {
    std::vector<CurrentVector> currents;

    std::vector<float> circleVertexValues = generate_coil_vertices_unrolled(torus.r2, torus.coilLoopSegments);
    std::vector<glm::vec4> circleVertices;
    for (int i = 0; i < torus.coilLoopSegments; i++) {
        circleVertices.push_back(glm::vec4 { circleVertexValues[i*2], circleVertexValues[i*2 + 1], 0.0f, 1.0f });
    }

    int idx = 0;
    int coilStartIdx = 0;
    for (int i = 0; i < torus.toroidalCoils; ++i) {
        coilStartIdx = idx;

        float angle = (2.0f * M_PI * i) / torus.toroidalCoils;
        glm::mat4 model = get_coil_model_matrix(angle, torus.r1);

        for (int j = 0; j < torus.coilLoopSegments; ++j) {
            CurrentVector current;
            current.x = model * circleVertices[j];
            current.i = torus.toroidalI;

            if (j > 0) currents[idx-1].dx = current.x - currents[idx-1].x;

            currents.push_back(current);
            idx++;
        }
        currents[idx-1].dx = currents[coilStartIdx].x - currents[idx-1].x;
    }

    return currents;
}

// Outside of a solenoid, the electric field is given by E_t = -1/(2*pi*r) d(Phi)/dt
// Assuming a pulse current of I_0*e^(-alpha*t), the electric field becomes E_t = 1/2 * alpha*u_0*N*I_0*R2 * 1/r * e^(-alpha*t)
// This function calculates that formula's constant
// https://openstax.org/books/university-physics-volume-2/pages/13-4-induced-electric-fields
float solenoid_pulse_e_field_parameter(TorusProperties& torus) {
    return 0.5f * torus.pulseAlpha * MU_0 * (float)torus.solenoidN * torus.solenoidI * (torus.solenoidR * torus.solenoidR);
}

float solenoid_pulse_e_field_multiplier(TorusProperties& torus, float t) {
    return solenoid_pulse_e_field_parameter(torus) * exp(-torus.pulseAlpha * t);
}