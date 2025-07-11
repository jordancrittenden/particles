#pragma once

#include <glm/glm.hpp>
#include "physical_constants.h"

glm::f32vec4 maxwell_boltzmann_particle_velocty(float T, float mass);

PARTICLE_SPECIES rand_particle_species(
    float partsNeutron,
    float partsElectron,
    float partsProton,
    float partsDeuterium,
    float partsTritium,
    float partsIonDeuterium,
    float partsIonTritium,
    float partsElectronMacroparticle,
    float partsProtonMacroparticle);