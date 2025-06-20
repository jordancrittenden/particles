#include <vector>
#include <iostream>
#include <random>
#include <cmath>
#include <glm/glm.hpp>
#include "physical_constants.h"

const double k_B = 1.380649e-23 * _J / _K; // Boltzmann constant (J/K)

inline float rand_range(float min, float max) {
    return static_cast<float>(rand()) / RAND_MAX * (max - min) + min;
}

glm::f32vec4 maxwell_boltzmann_particle_velocty(float T, float mass) {
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

    return glm::f32vec4 { vx, vy, vz, 0.0f };
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