#include "Scene.h"

#include <cmath>

void Scene::addObject(
    const Mesh &mesh,
    const Pipeline &pipeline,
    const Texture &texture,
    Transform transform,
    std::optional<OrbitalBody> orbital,
    std::string name,
    std::optional<uint32_t> parentIndex
) {
    objects.push_back(
        {&mesh, &pipeline, &texture, std::move(transform), std::move(orbital), parentIndex, std::move(name)}
    );
}

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
    }

    // Pass 2: has parent — parent position already updated in pass 1
    for (auto &obj: objects) {
        if (!obj.parentIndex || !obj.orbital) continue;

        auto &[radius, speed, angle] = *obj.orbital;
        angle += speed * deltaTime;

        obj.transform.position = objects[*obj.parentIndex].transform.position
                                 + glm::vec3{
                                     std::cos(angle) * radius,
                                     std::sin(angle) * radius,
                                     0.0f
                                 };
    }
}
