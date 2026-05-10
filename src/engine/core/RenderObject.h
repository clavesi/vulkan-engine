#pragma once

#include "Transform.h"
#include "Mesh.h"

struct RenderObject {
    const Mesh* mesh = nullptr;
    Transform transform;
};