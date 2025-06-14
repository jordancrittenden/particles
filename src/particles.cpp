#include <vector>
#include <iostream>
#include <random>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util/gl_util.h"
#include "util/cl_util.h"
#include "particles.h"

const double k_B = 1.380649e-23 * _J / _K; // Boltzmann constant (J/K)

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

cl_float4 maxwell_boltzmann_particle_velocty(float T, float mass) {
    // Define parameters for the Maxwell-Boltzmann distribution
    float sigma = std::sqrt(k_B * T / mass); // Standard deviation for the velocity distribution

    // Random number generation
    std::random_device rd;  // Seed for the random number engine
    std::mt19937 gen(rd()); // Mersenne Twister random number generator
    std::normal_distribution<> normal(0.0, sigma); // Gaussian distribution with mean 0 and std dev sigma

    // Sample velocities in 3D and calculate the magnitude
    float vx = normal(gen);
    float vy = normal(gen);
    float vz = normal(gen);

    return cl_float4 { vx, vy, vz, 0.0f };
}

PARTICLE_SPECIES rand_particle_species(float partsNeutron, float partsElectron, float partsProton, float partsDeuterium, float partsTritium, float partsIonDeuterium, float partsIonTritium) {
    float total = partsNeutron + partsElectron + partsProton + partsDeuterium + partsTritium + partsIonDeuterium + partsIonTritium;
    float rnd = rand_range(0.0, total);

    float level = partsNeutron;
    if (rnd < level) return NEUTRON;
    level += partsElectron;
    if (rnd < level) return ELECTRON;
    level += partsProton;
    if (rnd < level) return PROTON;
    level += partsDeuterium;
    if (rnd < level) return DEUTERIUM;
    level += partsTritium;
    if (rnd < level) return TRITIUM;
    level += partsIonDeuterium;
    if (rnd < level) return DEUTERON;
    level += partsIonTritium;
    if (rnd < level) return TRITON;

    std::cerr << "Invalid particle species" << std::endl;

    return NEUTRON;
}

// Create buffers for particle position/velocity
void create_particle_buffers(
    std::function<cl_float4()> posF,
    std::function<cl_float4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    GLBuffers& posBuf,
    GLBuffers& velBuf,
    int initialParticles,
    int maxParticles)
{
    std::vector<cl_float4> position_and_type;
    std::vector<cl_float4> velocity;

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < maxParticles; ++i) {
        if (i < initialParticles) {
            PARTICLE_SPECIES species = speciesF();
            cl_float4 pos = posF();
            cl_float4 vel = velF(species);

            pos.s[3] = (float)species;

            // [x, y, z, species]
            position_and_type.push_back(pos);
            // [dx, dy, dz, unused]
            velocity.push_back(vel);
        } else {
            // Placeholders for future particles that may be created via collisions

            // [x, y, z, species]
            position_and_type.push_back(cl_float4 { 0.0f, 0.0f, 0.0f, 0.0f });
            // [dx, dy, dz, unused]
            velocity.push_back(cl_float4 { 0.0f, 0.0f, 0.0f, 0.0f });
        }
    }

    // position/type buffer
    glGenVertexArrays(1, &posBuf.vao);
    glBindVertexArray(posBuf.vao);
    glGenBuffers(1, &posBuf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(cl_float4), position_and_type.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)0); // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(cl_float4), (void*)(3 * sizeof(float))); // charge attribute
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // velocity buffer
    glGenVertexArrays(1, &velBuf.vao);
    glBindVertexArray(velBuf.vao);
    glGenBuffers(1, &velBuf.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, velBuf.vbo);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(cl_float4), velocity.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection) {
    glUseProgram(shader);

    glm::mat4 model = glm::mat4(1.0f);

    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draw particles
    glBindVertexArray(posBuf.vao);
    glDrawArrays(GL_POINTS, 0, nParticles);
}