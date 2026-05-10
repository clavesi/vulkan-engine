#include "Scene.h"

#include <cmath>

void Scene::addObject(
    const Mesh &mesh,
    const Pipeline &pipeline,
    const Texture &texture,
    Transform transform,
    std::optional<OrbitalBody> orbital,
    std::string name
) {
    objects.push_back({&mesh, &pipeline, &texture, std::move(transform), std::move(orbital), std::move(name)});
}

void Scene::update(float deltaTime) {
    for (auto &obj: objects) {
        if (!obj.orbital) continue;

        auto &[radius, speed, angle] = *obj.orbital;
        angle += speed * deltaTime;

        // Circular orbit in xy plane around origin
        obj.transform.position = {
            std::cos(angle) * radius,
            std::sin(angle) * radius,
            0.0f
        };
    }
}
