#include "Scene.h"

#include <cmath>

SceneObject &Scene::addObject(const Mesh &mesh, const Pipeline &pipeline, Transform transform,
                              const std::optional<OrbitalBody> orbital) {
    return objects.emplace_back(SceneObject{
        .mesh = &mesh,
        .pipeline = &pipeline,
        .transform = transform,
        .orbital = orbital
    });
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
