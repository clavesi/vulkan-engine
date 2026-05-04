#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(const float distance, const float yaw, const float pitch,
               const float fov, const float nearPlane, const float farPlane)
    : distance(distance), yaw(yaw), pitch(pitch),
      fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
}

void Camera::onMouseDrag(const glm::vec2 delta) {
    yaw += delta.x * orbitSensitivity;
    pitch += delta.y * orbitSensitivity;

    // Clamp pitch so the camera doesn't flip over the poles
    pitch = std::clamp(pitch, pitchMin, pitchMax);
}

void Camera::onScroll(const float delta) {
    // Scroll up (positive delta) zooms in — reduce distance
    distance -= delta * scrollSensitivity * distance;
    // Prevent zooming through the target or to infinite distance
    distance = std::max(distance, 0.1f);
}

void Camera::setTarget(const glm::vec3 target) {
    this->target = target;
}

void Camera::setDistance(const float distance) {
    this->distance = distance;
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
