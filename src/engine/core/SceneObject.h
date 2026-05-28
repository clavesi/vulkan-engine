#pragma once

#include "Transform.h"
#include "OrbitalBody.h"
#include "Mesh.h"
#include "EarthMaterial.h"
#include "vk/Texture.h"
#include "app/BodyDef.h"

#include <optional>
#include <variant>

class Pipeline; // forward declare to avoid circular includes

struct SceneObject {
    const Mesh *mesh = nullptr;
    const Pipeline *pipeline = nullptr;
    std::variant<const Texture*, EarthMaterial> material = static_cast<const Texture*>(nullptr);
    Transform transform;
    std::optional<OrbitalBody> orbital; // absent = stationary
    std::optional<uint32_t> parentIndex;
    std::string name;
    const app::PlanetDef *bodyDef = nullptr; // null for sun and moons
    float rotationSpeedRads = 0.0f;
    glm::quat tiltRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);; // fixed axial tilt, set at creation
    float spinAngle = 0.0f; // accumulated spin around own axis
};
