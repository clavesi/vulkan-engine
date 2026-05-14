#pragma once

#include "core/Scene.h"
#include "core/Mesh.h"
#include "vk/Texture.h"
#include "vk/Device.h"
#include "vk/TextureLoader.h"

#include <list>
#include <vector>

namespace app {
    struct MoonDef {
        std::string name;
        float radiusKm;
        float periodDays;
        std::string texturePath;
    };

    struct PlanetDef {
        std::string name;
        float radiusKm; // real radius — will be scaled
        float orbitAu; // real orbital radius in AU — will be scaled
        float periodYears; // real orbital period in Earth years
        std::string texturePath;
        std::vector<MoonDef> moons;
    };

    class SolarSystem {
    public:
        // Scale factors
        static constexpr float SUN_RADIUS = 5.0f;
        static constexpr float AU_SCALE = 30.0f; // 1 AU -> 10 engine units (Earth orbit = 10)
        static constexpr float RADIUS_SCALE = 0.5f / 6371.0f; // Earth radius (6371km) -> 0.5 units
        static constexpr float PERIOD_SCALE = 0.05f; // 1 Earth year -> 0.1 seconds (speeds things up)
        // Moon orbits at ~60x Earth's radius in reality.
        // We want it at N x Earth's visual radius
        static constexpr float MOON_ORBIT_RADII = 5.0f; // N parent radii
        static constexpr float DAYS_PER_YEAR = 365.25f;

        static void init(Scene &scene, const Mesh &sphere,
                         const Pipeline &litPipeline, const Pipeline &unlitPipeline,
                         std::list<Texture> &textures, const Device &device);

    private:
        static const std::vector<PlanetDef> planets;

        static void addSun(Scene &scene, const Mesh &sphere, const Pipeline &unlitPipeline,
                           std::list<Texture> &textures, const Device &device);

        static void addPlanets(Scene &scene, const Mesh &sphere, const Pipeline &litPipeline,
                               std::list<Texture> &textures, const Device &device);
    };
} // namespace app
