#pragma once

#include <glm/glm.hpp>
#include "gl_util.h"

#define M_ELECTRON     (9.10938188e-31f) /* kg */
#define M_PROTON       (1.67262158e-27f) /* kg */
#define M_NEUTRON      (1.67492716e-27f) /* kg */
#define M_DEUTERIUM    (3.34449439e-27f) /* kg */
#define M_TRITIUM      (5.00826721e-27f) /* kg */
#define M_HELIUM_4_NUC (6.64647309e-27f) /* kg */
#define M_DEUTERON     (3.34358347e-27f) /* kg */
#define M_TRITON       (5.00735629e-27f) /* kg */

enum PARTICLE_SPECIES {
    NEUTRON = 1,
    ELECTRON = 2,
    PROTON = 3,
    DEUTERIUM = 4,
    TRITIUM = 5,
    HELIUM_4_NUC = 6,
    DEUTERON = 7,
    TRITON = 8
};

float particle_mass(PARTICLE_SPECIES species);
cl_float4 maxwell_boltzmann_particle_velocty(float T, float mass);
PARTICLE_SPECIES rand_particle_species(float partsNeutron, float partsElectron, float partsProton, float partsDeuterium, float partsTritium, float partsIonDeuterium, float partsIonTritium);
void create_particle_buffers(
    std::function<cl_float4()> posF,
    std::function<cl_float4(PARTICLE_SPECIES)> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    GLBuffers& posBuf,
    GLBuffers& velBuf,
    int initialParticles,
    int maxParticles);
void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection);