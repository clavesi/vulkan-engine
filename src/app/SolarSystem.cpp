#include "SolarSystem.h"
#include "core/OrbitalBody.h"
#include "core/Transform.h"

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
        scene.addObject(
            sphere, unlitPipeline, textures.back(),
            Transform{.scale = {SUN_RADIUS, SUN_RADIUS, SUN_RADIUS}},
            std::nullopt, "Sun"
        );

        // Planets
        std::unordered_map<std::string, uint32_t> planetIndices;
        uint32_t idx = 1; // sun is 0
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

            planetIndices[p.name] = idx++;
        }

        // Earth's moon
        textures.emplace_back();
        vk_util::loadTexture(device, "textures/solar/2k_moon.jpg", textures.back());
        const uint32_t earthIdx = planetIndices.at("Earth");
        constexpr float moonRadius = 1737.4f * RADIUS_SCALE;
        constexpr float parentRadius = 6371.0f * RADIUS_SCALE; // Earth's visual radius
        constexpr float moonOrbit = parentRadius * MOON_ORBIT_RADII;
        constexpr float moonSpeed = (glm::two_pi<float>() / (27.32f / DAYS_PER_YEAR)) * PERIOD_SCALE;
        scene.addObject(
            sphere, litPipeline, textures.back(),
            Transform{
                .position = scene.getObjects()[earthIdx].transform.position + glm::vec3{moonOrbit, 0.0f, 0.0f},
                .scale = {moonRadius, moonRadius, moonRadius}
            },
            OrbitalBody{.radius = moonOrbit, .speed = moonSpeed},
            "Moon",
            earthIdx
        );
    }
} // namespace app
