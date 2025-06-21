#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "cell.h"

glm::f32vec4 free_space_rand_particle_position(glm::f32vec3 minCoord, glm::f32vec3 maxCoord);

std::vector<Cell> get_free_space_grid_cells(glm::f32vec3 minCoord, glm::f32vec3 maxCoord, glm::f32 dx);