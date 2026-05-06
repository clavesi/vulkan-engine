#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance, float yaw, float pitch, float fov, float nearPlane, float farPlane);

    // Orbit controls
    void onMouseDrag(glm::vec2 delta);
    void onScroll(float delta);

    // Move orbit target (e.g. focus on planet)
    void setTarget(glm::vec3 target);
    void setDistance(float distance);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Current camera position in world space
    glm::vec3 getPosition() const;

private:
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    float distance;
    float yaw; // horizontal angle in radians
    float pitch; // vertical angle in radians

    float fov; // degrees
    float nearPlane;
    float farPlane;

    // How fast mouse drag rotates and scroll zooms
    static constexpr float orbitSensitivity = 0.005f;
    static constexpr float scrollSensitivity = 0.1f;

    // Prevent gimbal lock at the poles
    static constexpr float pitchMin = -1.5f; // just under -π/2
    static constexpr float pitchMax = 1.5f; // just under +π/2
};
