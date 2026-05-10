#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(const float distance, const float yaw, const float pitch,
               const float fov, const float nearPlane, const float farPlane)
    : distance(distance), desiredDistance(distance), defaultDistance(distance),
      yaw(yaw), desiredYaw(yaw), defaultYaw(yaw),
      pitch(pitch), desiredPitch(pitch), defaultPitch(pitch),
      fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
}

void Camera::update(float deltaTime) {
    // Exponential smoothing
    const float targetT = 1.0f - std::exp(-targetSmoothSpeed * deltaTime);
    const float distanceT = 1.0f - std::exp(-distanceSmoothSpeed * deltaTime);

    target = glm::mix(target, desiredTarget, targetT);
    distance = glm::mix(distance, desiredDistance, distanceT);
    yaw = glm::mix(yaw, desiredYaw, targetT);
    pitch = glm::mix(pitch, desiredPitch, targetT);
}

void Camera::onMouseDrag(const glm::vec2 delta) {
    if (mode == Mode::eFree) {
        freeCamYaw -= delta.x * freeLookSensitivity;
        freeCamPitch -= delta.y * freeLookSensitivity;
        freeCamPitch = std::clamp(freeCamPitch, pitchMin, pitchMax);
    } else {
        desiredYaw -= delta.x * orbitSensitivity;
        desiredPitch += delta.y * orbitSensitivity;
        desiredPitch = std::clamp(desiredPitch, pitchMin, pitchMax);
    }
}

void Camera::onScroll(const float delta) {
    if (mode == Mode::eFree) {
        // Scroll adjusts move speed multiplicatively
        moveSpeed *= std::pow(speedScrollScale, delta);
        moveSpeed = std::max(moveSpeed, 0.1f);
    } else {
        desiredDistance -= delta * scrollSensitivity * desiredDistance;
        desiredDistance = std::max(desiredDistance, 0.1f);
    }
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
    desiredYaw = defaultYaw;
    desiredPitch = defaultPitch;
    followObjectId = UINT32_MAX;
}

glm::mat4 Camera::getViewMatrix() const {
    if (mode == Mode::eFree) {
        const glm::vec3 forward = {
            std::cos(freeCamPitch) * std::cos(freeCamYaw),
            std::cos(freeCamPitch) * std::sin(freeCamYaw),
            std::sin(freeCamPitch)
        };
        return glm::lookAt(freeCamPos, freeCamPos + forward, glm::vec3{0.0f, 0.0f, 1.0f});
    }
    return glm::lookAt(getPosition(), target, glm::vec3{0.0f, 0.0f, 1.0f});
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

glm::vec3 Camera::getPosition() const {
    if (mode == Mode::eFree) {
        return freeCamPos;
    }

    // Spherical to Cartesian — yaw rotates around Z (up axis),
    // pitch tilts toward/away from the XY plane
    return target + glm::vec3{
               distance * std::cos(pitch) * std::cos(yaw),
               distance * std::cos(pitch) * std::sin(yaw),
               distance * std::sin(pitch)
           };
}

void Camera::toggleFreeCam() {
    if (mode == Mode::eOrbit) {
        // Inherit position and look direction from current orbit state
        freeCamPos = getPosition();
        // Orbit yaw points FROM target TO camera
        freeCamYaw = yaw + glm::pi<float>();
        freeCamPitch = -pitch;
        followObjectId = UINT32_MAX;
        mode = Mode::eFree;
    } else {
        mode = Mode::eOrbit;
    }
}

void Camera::onFreeCamMove(glm::vec3 localInput, float deltaTime) {
    if (mode != Mode::eFree || localInput == glm::vec3{0.0f}) {
        return;
    }

    // Build right/up/forward from free cam angles
    const glm::vec3 forward = {
        std::cos(freeCamPitch) * std::cos(freeCamYaw),
        std::cos(freeCamPitch) * std::sin(freeCamYaw),
        std::sin(freeCamPitch)
    };
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3{0.0f, 0.0f, 1.0f}));
    const glm::vec3 up = glm::cross(right, forward);

    freeCamPos += (right * localInput.x + forward * localInput.y + up * localInput.z) * moveSpeed * deltaTime;
}
