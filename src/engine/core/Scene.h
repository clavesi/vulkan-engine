#pragma once

#include "SceneObject.h"
#include "OrbitCircle.h"

#include <vector>

class Scene {
public:
    void update(float deltaTime);

    // Add an object to the scene.
    void addObject(
        const Mesh &mesh,
        const Pipeline &pipeline,
        std::variant<const Texture *, EarthMaterial> material,
        Transform transform = {},
        std::optional<OrbitalBody> orbital = std::nullopt,
        std::string name = "",
        std::optional<uint32_t> parentIndex = std::nullopt,
        const app::PlanetDef *bodyDef = nullptr,
        float rotationSpeedRads = 0.0f,
        glm::quat tiltRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    const std::vector<SceneObject> &getObjects() const { return objects; }

    void addOrbitCircle(const Mesh &mesh, glm::vec3 color);
    const std::vector<OrbitCircle> &getOrbitCircles() const { return orbitCircles; }

private:
    std::vector<SceneObject> objects;
    std::vector<OrbitCircle> orbitCircles;
};
