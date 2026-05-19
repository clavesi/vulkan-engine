#pragma once

#include "core/Scene.h"
#include "core/Mesh.h"
#include "vk/Texture.h"
#include "app/BodyDef.h"

#include <list>
#include <vector>

namespace app {
    class SolarSystem {
    public:
        // Scale factors
        static constexpr float SUN_RADIUS = 5.0f;
        static constexpr float AU_SCALE = 30.0f; // 1 AU -> 10 engine units (Earth orbit = 10)
        static constexpr float RADIUS_SCALE = 0.5f / 6371.0f; // Earth radius (6371km) -> 0.5 units
        static constexpr float PERIOD_SCALE = 0.01f; // higher value speeds up simulation time
        static constexpr float DAYS_PER_YEAR = 365.25f;

        static constexpr int ASTEROID_BELT_COUNT = 1500;

        static void init(
            Scene &scene, const Mesh &sphere, const Mesh &asteroidMesh,
            const Pipeline &litPipeline, const Pipeline &unlitPipeline,
            const Pipeline &earthPipeline, std::list<Texture> &textures,
            const Device &device, std::list<Mesh> &orbitMeshes
        );

    private:
        static const std::vector<PlanetDef> planets;

        static void addSun(Scene &scene, const Mesh &sphere, const Pipeline &unlitPipeline,
                           std::list<Texture> &textures, const Device &device);

        static void addPlanets(Scene &scene, const Mesh &sphere, const Pipeline &litPipeline,
                               const Pipeline &earthPipeline,
                               std::list<Texture> &textures, const Device &device, std::list<Mesh> &orbitMeshes);

        static void addAsteroidBelt(Scene &scene, const Mesh &asteroidMesh, const Pipeline &litPipeline,
                                    std::list<Texture> &textures, const Device &device);
    };
} // namespace app
