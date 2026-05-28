#pragma once

#include "Mesh.h"
#include <glm/glm.hpp>

struct OrbitCircle {
    const Mesh* mesh =nullptr;
    glm::vec3 color;
};