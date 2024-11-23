#include <iostream>
#include "state.h"

void print_state(const SimulationState& state) {
    std::cout << "Simulation State:" << std::endl;
    std::cout << "  initial particles:" << state.nInitialParticles << std::endl;
    std::cout << "  interparticle physics: " << (state.enableInterparticlePhysics ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  toroidal rings: " << (state.enableToroidalRings ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  solenoid: " << (state.enableSolenoidFlux ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "  dt:" << state.dt << std::endl;
}