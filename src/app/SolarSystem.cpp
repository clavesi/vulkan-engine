#include "SolarSystem.h"
#include "core/MeshGenerator.h"
#include "core/OrbitalBody.h"
#include "core/Transform.h"
#include "vk/TextureLoader.h"
#include "io/GltfLoader.h"

#include <random>

namespace app {
    const std::vector<PlanetDef> SolarSystem::planets = {
        {"Mercury", 2439.7f, 0.387f, 87.9691f, 58.646f, 0.034f, "textures/solar/2k_mercury.jpg"},
        {"Venus", 6051.8f, 0.723f, 224.701f, -243.0226f, 177.36f, "textures/solar/2k_venus_atmosphere.jpg"},
        {
            "Earth", 6371.0f, 1.000f, 365.256f, 0.9973f, 23.4393f, "textures/solar/2k_earth_daymap.jpg", {
                {"Moon", 1737.4f, 384400.0f, 27.32f, "textures/solar/2k_moon.jpg"},
            }
        },
        {
            "Mars", 3389.5f, 1.524f, 686.980f, 1.026f, 25.19f, "textures/solar/2k_mars.jpg", {
                {"Phobos", 11.267f, 9376.0f, 0.319f, "textures/solar/mars-phobos.jpg"},
                {"Deimos", 6.2f, 23463.0f, 1.263f, "textures/solar/mars-deimos.jpg"},
            }
        },
        {
            "Jupiter", 69911.0f, 5.203f, 4332.59f, 0.4147f, 3.13f, "textures/solar/2k_jupiter.jpg", {
                {"Io", 1821.6f, 421800.0f, 1.769f, "textures/solar/jupiter-io.jpg"},
                {"Europa", 1560.8f, 671100.0f, 3.551f, "textures/solar/jupiter-europa.jpg"},
                {"Ganymede", 2634.1f, 1070400.0f, 7.155f, "textures/solar/jupiter-ganymede.jpg"},
                {"Callisto", 2410.3f, 1882700.0f, 16.69f, "textures/solar/jupiter-callisto.jpg"},
            }
        },
        {"Saturn", 58232.0f, 9.537f, 10755.70f, 0.44f, 26.73f, "textures/solar/2k_saturn.jpg"},
        {"Uranus", 25362.0f, 19.19f, 30688.5, -0.7187f, 82.23f, "textures/solar/2k_uranus.jpg"},
        {"Neptune", 24622.0f, 30.07f, 60195.0f, 0.673f, 28.32f, "textures/solar/2k_neptune.jpg"},
    };

    static const std::vector<glm::vec3> planetColors = {
        {0.6f, 0.6f, 0.6f}, // Mercury — grey
        {0.9f, 0.8f, 0.5f}, // Venus — yellowish
        {0.2f, 0.5f, 0.9f}, // Earth — blue
        {0.8f, 0.4f, 0.2f}, // Mars — red
        {0.8f, 0.7f, 0.5f}, // Jupiter — tan
        {0.9f, 0.8f, 0.6f}, // Saturn — pale gold
        {0.5f, 0.8f, 0.9f}, // Uranus — cyan
        {0.3f, 0.4f, 0.9f}, // Neptune — blue
    };

    void SolarSystem::init(
        Scene &scene, const Mesh &sphere, const Mesh &asteroidMesh,
        const Pipeline &litPipeline, const Pipeline &unlitPipeline,
        std::list<Texture> &textures, const Device &device,
        std::list<Mesh> &orbitMeshes
    ) {
        addSun(scene, sphere, unlitPipeline, textures, device);
        addPlanets(scene, sphere, litPipeline, textures, device, orbitMeshes);
        addAsteroidBelt(scene, asteroidMesh, litPipeline, textures, device);
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
        std::list<Texture> &textures, const Device &device,
        std::list<Mesh> &orbitMeshes
    ) {
        // Sun is index 0, planets start at 1
        uint32_t idx = 1;
        size_t colorIdx = 0;
        for (const auto &planet: planets) {
            textures.emplace_back();
            vk_util::loadTexture(device, planet.texturePath, textures.back());

            const float r = planet.radiusKm * RADIUS_SCALE;
            const float planetOrbit = planet.orbitAu * AU_SCALE;
            const float planetSpeed = (glm::two_pi<float>() / (planet.periodDays / DAYS_PER_YEAR)) * PERIOD_SCALE;
            const float rotSpeed = (glm::two_pi<float>() / (planet.rotationDays / DAYS_PER_YEAR)) * PERIOD_SCALE;
            const glm::quat tilt = glm::angleAxis(
                glm::radians(planet.axialTiltDeg),
                glm::vec3{1.0f, 0.0f, 0.0f}
            );

            scene.addObject(
                sphere, litPipeline, textures.back(),
                Transform{
                    .position = {
                        planetOrbit, 0.0f, 0.0f
                    },
                    .scale = {r, r, r}
                },
                OrbitalBody{.radius = planetOrbit, .speed = planetSpeed},
                planet.name,
                std::nullopt,
                &planet,
                rotSpeed,
                tilt
            );

            // Orbit circle
            auto [verts, indices] = MeshGenerator::circle(planetOrbit, 128);
            orbitMeshes.emplace_back(device, verts, indices);
            scene.addOrbitCircle(orbitMeshes.back(), planetColors[colorIdx++]);

            const uint32_t planetIdx = idx++;
            const float parentRadius = r; // already scaled

            for (const auto &moon: planet.moons) {
                textures.emplace_back();
                vk_util::loadTexture(device, moon.texturePath, textures.back());

                const float mr = moon.radiusKm * RADIUS_SCALE;
                // Real ratio: how many parent radii this moon orbits at, compressed for visibility
                const float realRatio = moon.orbitKm / planet.radiusKm;
                const float moonOrbit = std::log(realRatio + 1.0f) * parentRadius * 1.5f;
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

    void SolarSystem::addAsteroidBelt(
        Scene &scene, const Mesh &asteroidMesh, const Pipeline &litPipeline,
        std::list<Texture> &textures, const Device &device
    ) {
        textures.emplace_back();
        // Texture already loaded from glb — reuse same texture for all asteroids
        // Load once, store once, all asteroids point to it
        if (!io::loadGltfBaseColorTexture(device, "models/asteroid-low-poly/asteroid.glb", textures.back())) {
            vk_util::loadTexture(device, "textures/solar/2k_moon.jpg", textures.back());
        }
        const Texture &asteroidTex = textures.back();

        // Seed for reproducible belt
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
        std::normal_distribution<float> orbitDist(2.7f * AU_SCALE, 0.4f * AU_SCALE);
        std::uniform_real_distribution<float> scaleDist(0.0003f, 0.001f);
        std::uniform_real_distribution<float> speedDist(0.6f, 1.2f); // relative to a 2.7 AU orbit
        std::uniform_real_distribution<float> zDist(-0.3f, 0.3f); // slight vertical spread

        // Base orbital speed at 2.7 AU (Kepler: speed ∝ 1/sqrt(r))
        constexpr float baseOrbit = 2.7f * AU_SCALE;
        constexpr float baseSpeed = (glm::two_pi<float>() / (1643.5f / DAYS_PER_YEAR)) * PERIOD_SCALE;

        for (int i = 0; i < ASTEROID_BELT_COUNT; ++i) {
            const float orbit = std::clamp(orbitDist(rng), 1.8f * AU_SCALE, 3.2f * AU_SCALE);
            const float angle = angleDist(rng);
            const float s = scaleDist(rng);
            // Kepler scaling: objects further out orbit slower
            const float speed = baseSpeed * std::sqrt(baseOrbit / orbit) * speedDist(rng);
            const float zOffset = zDist(rng);

            const glm::vec3 pos = {
                std::cos(angle) * orbit,
                std::sin(angle) * orbit,
                zOffset
            };

            // Random initial rotation
            std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());
            const glm::quat rot = glm::angleAxis(rotDist(rng), glm::normalize(glm::vec3{
                                                     rotDist(rng), rotDist(rng), rotDist(rng)
                                                 }));

            scene.addObject(
                asteroidMesh, litPipeline, asteroidTex,
                Transform{.position = pos, .rotation = rot, .scale = {s, s, s}},
                OrbitalBody{.radius = orbit, .speed = speed, .angle = angle}
                // no name — won't show in UI
            );
        }
    }
} // namespace app
