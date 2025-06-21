#pragma once

#include <functional>
#include <glm/glm.hpp>
#include "util/cl_util.h"
#include "util/gl_util.h"
#include "physical_constants.h"

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