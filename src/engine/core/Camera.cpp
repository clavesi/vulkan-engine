#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(const float distance, const float yaw, const float pitch,
               const float fov, const float nearPlane, const float farPlane)
    : distance(distance), desiredDistance(distance), defaultDistance(distance),
      yaw(yaw), defaultYaw(yaw), pitch(pitch), defaultPitch(pitch),
      fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
}

void Camera::update(float deltaTime) {
    // Exponential smoothing — fast initial movement that decelerates naturally
    const float targetT = 1.0f - std::exp(-targetSmoothSpeed * deltaTime);
    const float distanceT = 1.0f - std::exp(-distanceSmoothSpeed * deltaTime);

    target = glm::mix(target, desiredTarget, targetT);
    distance = glm::mix(distance, desiredDistance, distanceT);
}

void Camera::onMouseDrag(const glm::vec2 delta) {
    yaw -= delta.x * orbitSensitivity; // horizontal
    pitch += delta.y * orbitSensitivity; // vertical

    // Clamp pitch so the camera doesn't flip over the poles
    pitch = std::clamp(pitch, pitchMin, pitchMax);
}

void Camera::onScroll(const float delta) {
    // Scroll up (positive delta) zooms in — reduce distance
    desiredDistance -= delta * scrollSensitivity * desiredDistance;
    // Prevent zooming through the target or to infinite distance
    desiredDistance = std::max(desiredDistance, 0.1f);
}

void Camera::setTarget(const glm::vec3 target) {
    this->target = target;
}

void Camera::setDesiredTarget(const glm::vec3 newTarget) {
    desiredTarget = newTarget;
}

void Camera::setTargetImmediate(const glm::vec3 target) {
    this->target = target;
    this->desiredTarget = target;
}

void Camera::setDistance(const float distance) {
    this->distance = distance;
}

void Camera::setDesiredDistance(const float newDistance) {
    desiredDistance = newDistance;
}

void Camera::resetTarget() {
    desiredTarget = {0.0f, 0.0f, 0.0f};
    desiredDistance = defaultDistance;
    yaw = defaultYaw;
    pitch = defaultPitch;
    followObjectId = UINT32_MAX;
}

glm::vec3 Camera::getPosition() const {
    // Spherical to Cartesian — yaw rotates around Z (up axis),
    // pitch tilts toward/away from the XY plane
    return target + glm::vec3{
               distance * std::cos(pitch) * std::cos(yaw),
               distance * std::cos(pitch) * std::sin(yaw),
               distance * std::sin(pitch)
           };
}

glm::mat4 Camera::getViewMatrix() const {
    const glm::vec3 position = getPosition();
    // +Z is up in our scene
    return glm::lookAt(position, target, glm::vec3{0.0f, 0.0f, 1.0f});
}

glm::mat4 Camera::getProjectionMatrix(const float aspectRatio) const {
    glm::mat4 proj = glm::perspective(
        glm::radians(fov),
        aspectRatio,
        nearPlane,
        farPlane
    );
    proj[1][1] *= -1; // GLM was designed for OpenGL; Vulkan's Y axis is flipped
    return proj;
}
