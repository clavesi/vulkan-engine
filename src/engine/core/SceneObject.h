#pragma once

#include "Transform.h"
#include "OrbitalBody.h"
#include "Mesh.h"
#include "vk/Texture.h"

#include <optional>
#include <string>

class Pipeline; // forward declare to avoid circular includes

struct SceneObject {
    const Mesh *mesh = nullptr;
    const Pipeline *pipeline = nullptr;
    const Texture *texture = nullptr;
    Transform transform;
    std::optional<OrbitalBody> orbital; // absent = stationary
    std::string name;
};
