#include "SolarSystem.h"
#include "core/OrbitalBody.h"
#include "core/Transform.h"

namespace app {
    const std::vector<PlanetDef> SolarSystem::planets = {
        {"Mercury", 2439.7f, 0.387f, 0.241f, "textures/solar/2k_mercury.jpg"},
        {"Venus", 6051.8f, 0.723f, 0.615f, "textures/solar/2k_venus_atmosphere.jpg"},
        {
            "Earth", 6371.0f, 1.000f, 1.000f, "textures/solar/2k_earth_daymap.jpg", {
                {"Moon", 1737.4f, 27.32f, "textures/solar/2k_moon.jpg"},
            }
        },
        {
            "Mars", 3389.5f, 1.524f, 1.881f, "textures/solar/2k_mars.jpg", {
                {"Phobos", 11.267f, 0.319f, "textures/solar/2k_moon.jpg"},
                {"Deimos", 6.2f, 1.263f, "textures/solar/2k_moon.jpg"},
            }
        },
        {
            "Jupiter", 69911.0f, 5.203f, 11.86f, "textures/solar/2k_jupiter.jpg", {
                {"Io", 1821.6f, 1.769f, "textures/solar/2k_moon.jpg"},
                {"Europa", 1560.8f, 3.551f, "textures/solar/2k_moon.jpg"},
                {"Ganymede", 2634.1f, 7.155f, "textures/solar/2k_moon.jpg"},
                {"Callisto", 2410.3f, 16.69f, "textures/solar/2k_moon.jpg"},
            }
        },
        {"Saturn", 58232.0f, 9.537f, 29.46f, "textures/solar/2k_saturn.jpg"},
        {"Uranus", 25362.0f, 19.19f, 84.01f, "textures/solar/2k_uranus.jpg"},
        {"Neptune", 24622.0f, 30.07f, 164.8f, "textures/solar/2k_neptune.jpg"},
    };

    void SolarSystem::init(
        Scene &scene, const Mesh &sphere,
        const Pipeline &litPipeline, const Pipeline &unlitPipeline,
        std::list<Texture> &textures, const Device &device
    ) {
        addSun(scene, sphere, unlitPipeline, textures, device);
        addPlanets(scene, sphere, litPipeline, textures, device);
    }

    void SolarSystem::addSun(
        Scene &scene, const Mesh &sphere, const Pipeline &unlitPipeline,
        std::list<Texture> &textures, const Device &device
    ) {
        textures.emplace_back();
        vk_util::loadTexture(device, "textures/solar/2k_sun.jpg", textures.back());
        scene.addObject(
            sphere, unlitPipeline, textures.back(),
            Transform{.scale = {SUN_RADIUS, SUN_RADIUS, SUN_RADIUS}},
            std::nullopt, "Sun"
        );
    }

    void SolarSystem::addPlanets(
        Scene &scene, const Mesh &sphere, const Pipeline &litPipeline,
        std::list<Texture> &textures, const Device &device
    ) {
        // Sun is index 0, planets start at 1
        uint32_t idx = 1;
        for (const auto &planet: planets) {
            textures.emplace_back();
            vk_util::loadTexture(device, planet.texturePath, textures.back());

            const float r = planet.radiusKm * RADIUS_SCALE;
            const float planetOrbit = planet.orbitAu * AU_SCALE;
            const float planetSpeed = (glm::two_pi<float>() / planet.periodYears) * PERIOD_SCALE;

            scene.addObject(
                sphere, litPipeline, textures.back(),
                Transform{.position = {planetOrbit, 0.0f, 0.0f}, .scale = {r, r, r}},
                OrbitalBody{.radius = planetOrbit, .speed = planetSpeed},
                planet.name
            );

            const uint32_t planetIdx = idx++;
            const float parentRadius = r; // already scaled

            for (const auto &moon: planet.moons) {
                textures.emplace_back();
                vk_util::loadTexture(device, moon.texturePath, textures.back());

                const float mr = moon.radiusKm * RADIUS_SCALE;
                const float moonOrbit = parentRadius * MOON_ORBIT_RADII;
                const float moonSpeed = (glm::two_pi<float>() / (moon.periodDays / DAYS_PER_YEAR)) * PERIOD_SCALE;

                const glm::vec3 parentPos = scene.getObjects()[planetIdx].transform.position;

                scene.addObject(
                    sphere, litPipeline, textures.back(),
                    Transform{
                        .position = parentPos + glm::vec3{moonOrbit, 0.0f, 0.0f},
                        .scale = {mr, mr, mr}
                    },
                    OrbitalBody{.radius = moonOrbit, .speed = moonSpeed},
                    moon.name,
                    planetIdx
                );

                ++idx; // moon also occupies a scene index
            }
        }
    }
} // namespace app
