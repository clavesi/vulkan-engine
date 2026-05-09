#pragma once

#include "SceneObject.h"

#include <vector>

class Scene {
public:
    // Add an object to the scene.
    void addObject(
        const Mesh &mesh,
        const Pipeline &pipeline,
        const Texture &texture,
        Transform transform = {},
        std::optional<OrbitalBody> orbital = std::nullopt
    );

    void update(float deltaTime);

    const std::vector<SceneObject> &getObjects() const { return objects; }

private:
    std::vector<SceneObject> objects;
};
