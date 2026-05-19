#pragma once

class Texture;

struct EarthMaterial {
    const Texture *day = nullptr;
    const Texture *night = nullptr;
    const Texture *normal = nullptr;
    const Texture *specular = nullptr;
    const Texture *clouds = nullptr;
};
