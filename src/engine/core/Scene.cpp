#include "Scene.h"

#include <cmath>

void Scene::update(float deltaTime) {
    // Pass 1: no parent
    for (auto &obj: objects) {
        if (obj.parentIndex || !obj.orbital) continue;

        auto &[radius, speed, angle] = *obj.orbital;
        angle += speed * deltaTime;

        obj.transform.position = {
            std::cos(angle) * radius,
            std::sin(angle) * radius,
            0.0f
        };

        if (obj.rotationSpeedRads != 0.0f) {
            obj.spinAngle += obj.rotationSpeedRads * deltaTime;
            const glm::quat spin = glm::angleAxis(obj.spinAngle, glm::vec3{0.0f, 0.0f, 1.0f});
            obj.transform.rotation = obj.tiltRotation * spin;
        }
    }

    // Pass 2: has parent
    for (auto &obj: objects) {
        if (!obj.parentIndex) continue;

        if (obj.orbital) {
            // Moon-style: orbit around parent
            auto &[radius, speed, angle] = *obj.orbital;
            angle += speed * deltaTime;
            obj.transform.position = objects[*obj.parentIndex].transform.position + glm::vec3{
                                         std::cos(angle) * radius,
                                         std::sin(angle) * radius,
                                         0.0f
                                     };
        } else {
            // Ring-style: just follow parent position with no orbit
            obj.transform.position = objects[*obj.parentIndex].transform.position;
            obj.transform.rotation = objects[*obj.parentIndex].transform.rotation;
        }
    }
}

void Scene::addObject(
    const Mesh &mesh,
    const Pipeline &pipeline,
    std::variant<const Texture *, EarthMaterial> material,
    Transform transform,
    std::optional<OrbitalBody> orbital,
    std::string name,
    const std::optional<uint32_t> parentIndex,
    const app::PlanetDef *bodyDef,
    const float rotationSpeedRads,
    const glm::quat tiltRotation
) {
    objects.push_back(
        {
            &mesh, &pipeline, std::move(material), std::move(transform), std::move(orbital), parentIndex,
            std::move(name), bodyDef,
            rotationSpeedRads, tiltRotation
        }
    );
}

void Scene::addOrbitCircle(const Mesh &mesh, const glm::vec3 color) {
    orbitCircles.push_back({&mesh, color});
}
