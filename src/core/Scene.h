#pragma once

#include "SceneObject.h"

#include <vector>

class Scene {
public:
    // Add an object to the scene.
    // Returns a reference so the caller can store it and update its transform later
    SceneObject &addObject(const Mesh &mesh, const Pipeline &pipeline, Transform transform = {},
                           std::optional<OrbitalBody> orbital = std::nullopt);

    void update(float deltaTime);

    const std::vector<SceneObject> &getObjects() const { return objects; }

private:
    std::vector<SceneObject> objects;
};
