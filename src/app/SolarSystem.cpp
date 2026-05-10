#include "SolarSystem.h"
#include "core/OrbitalBody.h"
#include "core/Transform.h"

#include <glm/gtc/constants.hpp>

namespace app {
    const std::vector<PlanetDef> SolarSystem::planets = {
        {"Mercury", 2439.7f, 0.387f, 0.241f, "textures/solar/2k_mercury.jpg"},
        {"Venus", 6051.8f, 0.723f, 0.615f, "textures/solar/2k_venus_atmosphere.jpg"},
        {"Earth", 6371.0f, 1.000f, 1.000f, "textures/solar/2k_earth_daymap.jpg"},
        {"Mars", 3389.5f, 1.524f, 1.881f, "textures/solar/2k_mars.jpg"},
        {"Jupiter", 69911.0f, 5.203f, 11.86f, "textures/solar/2k_jupiter.jpg"},
        {"Saturn", 58232.0f, 9.537f, 29.46f, "textures/solar/2k_saturn.jpg"},
        {"Uranus", 25362.0f, 19.19f, 84.01f, "textures/solar/2k_uranus.jpg"},
        {"Neptune", 24622.0f, 30.07f, 164.8f, "textures/solar/2k_neptune.jpg"},
    };

    void SolarSystem::init(
        Scene &scene, const Mesh &sphere,
        const Pipeline &litPipeline, const Pipeline &unlitPipeline,
        std::list<Texture> &textures, const Device &device
    ) {
        // Sun
        textures.emplace_back();
        vk_util::loadTexture(device, "textures/solar/2k_sun.jpg", textures.back());
        constexpr float sunRadius = SUN_RADIUS;
        scene.addObject(
            sphere, unlitPipeline, textures.back(),
            Transform{.scale = {sunRadius, sunRadius, sunRadius}},
            std::nullopt, "Sun"
        );

        // Planets
        for (const auto &p: planets) {
            textures.emplace_back();
            vk_util::loadTexture(device, p.texturePath, textures.back());

            const float r = p.radiusKm * RADIUS_SCALE;
            const float orbit = p.orbitAu * AU_SCALE;
            const float speed = (glm::two_pi<float>() / p.periodYears) * PERIOD_SCALE;

            scene.addObject(
                sphere, litPipeline, textures.back(),
                Transform{.position = {orbit, 0.0f, 0.0f}, .scale = {r, r, r}},
                OrbitalBody{.radius = orbit, .speed = speed},
                p.name
            );
        }
    }
} // namespace app
