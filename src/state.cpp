#include <iostream>
#include "state.h"

void print_state(const SimulationState& state) {
    std::cout << "Simulation State:" << std::endl;
    std::cout << "  windowWidth:" << state.windowWidth << std::endl;
    std::cout << "  windowHeight:" << state.windowHeight << std::endl;
    std::cout << "  N:" << state.N << std::endl;
    std::cout << "  dt:" << state.dt << std::endl;
    std::cout << "  pos.vbo:" << state.pos.vbo << std::endl;
    std::cout << "  vel.vbo:" << state.vel.vbo << std::endl;
}