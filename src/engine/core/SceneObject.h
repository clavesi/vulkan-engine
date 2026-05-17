#pragma once

#include "Transform.h"
#include "OrbitalBody.h"
#include "Mesh.h"
#include "vk/Texture.h"
#include "app/BodyDef.h"

#include <optional>

class Pipeline; // forward declare to avoid circular includes

struct SceneObject {
    const Mesh *mesh = nullptr;
    const Pipeline *pipeline = nullptr;
    const Texture *texture = nullptr;
    Transform transform;
    std::optional<OrbitalBody> orbital; // absent = stationary
    std::optional<uint32_t> parentIndex;
    std::string name;
    const app::PlanetDef *bodyDef = nullptr; // null for sun and moons
    float rotationSpeedRads = 0.0f;
};
