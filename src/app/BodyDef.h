#pragma once

#include <string>
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
        float periodDays; // real orbital period in Earth days
        float rotationDays; // length of one day in Earth days
        std::string texturePath;
        std::vector<MoonDef> moons;
    };
}