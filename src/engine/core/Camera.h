#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(float distance, float yaw, float pitch, float fov, float nearPlane, float farPlane);

    void update(float deltaTime);

    // Orbit controls
    void onMouseDrag(glm::vec2 delta);
    void onScroll(float delta);

    // Move orbit target (e.g. focus on planet)
    void setTarget(glm::vec3 target);
    void setDesiredTarget(glm::vec3 newTarget);
    void setTargetImmediate(glm::vec3 target);
    void setDistance(float distance);
    void setDesiredDistance(float newDistance);

    // Reset to origin instantly
    void resetTarget();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Current camera position in world space
    glm::vec3 getPosition() const;

    void setFollowTarget(uint32_t objectId) { followObjectId = objectId; }
    void clearFollow() { followObjectId = UINT32_MAX; }
    bool isFollowing() const { return followObjectId != UINT32_MAX; }
    uint32_t getFollowObjectId() const { return followObjectId; }

private:
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    glm::vec3 desiredTarget = {0.0f, 0.0f, 0.0f};
    float distance;
    float desiredDistance = 3.0f;
    float defaultDistance;
    float yaw; // horizontal angle in radians
    float defaultYaw;
    float pitch; // vertical angle in radians
    float defaultPitch;

    float fov; // degrees
    float nearPlane;
    float farPlane;

    uint32_t followObjectId = UINT32_MAX;

    // How fast mouse drag rotates and scroll zooms
    static constexpr float orbitSensitivity = 0.005f;
    static constexpr float scrollSensitivity = 0.1f;

    // Prevent gimbal lock at the poles
    static constexpr float pitchMin = -1.5f; // just under -π/2
    static constexpr float pitchMax = 1.5f; // just under +π/2

    // How fast the camera interpolates — higher = snappier
    static constexpr float targetSmoothSpeed = 8.0f;
    static constexpr float distanceSmoothSpeed = 8.0f;
};
