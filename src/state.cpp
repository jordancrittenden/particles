#include <iostream>
#include "state.h"

void print_state(const SimulationState& state) {
    std::cout << "Simulation State:" << std::endl;
    std::cout << "  N:" << state.N << std::endl;
    std::cout << "  dt:" << state.dt << std::endl;
}