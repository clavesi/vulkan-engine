#pragma once

#include "Transform.h"
#include "OrbitalBody.h"
#include "Mesh.h"

#include <optional>

class Pipeline; // forward declare to avoid circular includes

struct SceneObject {
    const Mesh *mesh = nullptr;
    const Pipeline *pipeline = nullptr;
    Transform transform;
    std::optional<OrbitalBody> orbital; // absent = stationary
};
