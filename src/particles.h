#pragma once

#include <glm/glm.hpp>
#include "gl_util.h"

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

cl_float4 rand_particle_velocity();
PARTICLE_SPECIES rand_particle_species(float partsNeutron, float partsElectron, float partsProton, float partsDeuterium, float partsTritium, float partsIonDeuterium, float partsIonTritium);
void create_particle_buffers(
    std::function<cl_float4()> posF,
    std::function<cl_float4()> velF,
    std::function<PARTICLE_SPECIES()> speciesF,
    GLBuffers& posBuf,
    GLBuffers& velBuf,
    int nParticles);
void render_particles(GLuint shader, GLBuffers& posBuf, int nParticles, glm::mat4 view, glm::mat4 projection);