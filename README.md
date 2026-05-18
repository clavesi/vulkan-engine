# Vulkan Engine

A Vulkan rendering engine built while following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/),
extensively refactored into a modular engine and extended well beyond the tutorial into original engine work.

## Current State

The tutorial phase is complete. The engine has moved on to original architecture work in pursuit of a solar system
simulation: orbit camera, scene graph, per-object transforms, GPU picking, hover highlighting, ImGui UI, per-object
textures, planet axial rotation, moons with parented orbits, orbit circle visualization, asteroid belt, skybox, and
glTF model loading.

## Project Structure

```
src/engine/
  core/       — Camera, Input, Scene, SceneObject, Mesh, Transform, OrbitalBody, PipelineSpecs, config
  vk/         — Vulkan wrappers: Instance, Device, SwapChain, Pipeline, Renderer, Buffer, Image, Texture, TextureLoader
  io/         — File reading, OBJ loading, glTF loading
  ui/         — ImGui integration
  app/        — Solar system: SolarSystem, BodyDef
shaders/      — HLSL source + compiled SPIR-V
textures/
  solar/      — Planet, moon, sun, and skybox texture maps
models/       — glTF/GLB models (asteroid belt)
```

## Architecture

### Construction ordering

Member declaration order in `Engine` determines construction and destruction order. Key constraints:

- `Input` before `Window` — input callbacks fire during `glfwDestroyWindow`, so Input must outlive Window
- `Input::init()` before `imguiRenderer.initGlfw()` — ImGui chains onto GLFW callbacks set by Input
- `renderer.onSceneReady()` called after `initScene()` — descriptor sets must be allocated after the scene is populated

### Per-object textures

Each `SceneObject` holds a `Texture*`. `Renderer` allocates one descriptor set per object per frame
(`descriptorSets[objectIndex][frameIndex]`), each pointing at the shared UBO and that object's texture.
`Engine` owns textures in a `std::list<Texture>` — list nodes never move on insertion, keeping pointers stable.
Meshes are similarly owned in a `std::list<Mesh>`.

### GPU picking

A picking pass runs every frame before the main pass, rendering each object's index as a `uint` into an
`R32_UINT` image. The pixel under the cursor is copied to a host-visible readback buffer and read back
the following frame after the fence. Cursor coordinates are multiplied by `contentScale` for HiDPI displays.

### Scene update

`Scene::update` runs in two passes: parents (planets, sun) first, then children (moons). This ensures moon
positions are computed relative to their parent's already-updated position. Parent relationship is stored as
`std::optional<uint32_t> parentIndex` on `SceneObject`.

### Camera

Orbit camera with smooth exponential interpolation on yaw, pitch, distance, and target. Double-clicking an object
locks the camera to follow it. ESC resets to the default view. Press F to toggle free camera mode — WASD to move,
right drag to look, scroll to adjust speed.

### Solar system scaling

Planet orbital radii use `AU_SCALE` (1 AU = 30 engine units). Planet sizes use `RADIUS_SCALE` (Earth = 0.5 units).
Time uses `PERIOD_SCALE` expressed as engine seconds per Earth year. Moon orbits use logarithmic compression relative
to parent radius so all moon systems are visibly separated regardless of planet size.

## Controls

| Input               | Action                       |
|---------------------|------------------------------|
| Right mouse drag    | Orbit / free look            |
| Scroll              | Zoom / adjust fly speed      |
| Double-click object | Follow object                |
| ESC                 | Reset camera / exit free cam |
| SPACE               | Pause / resume simulation    |
| F                   | Toggle free camera           |
| WASD / E / Q        | Move in free camera          |

## Build

### Dependencies

- CMake >= 3.29
- GCC 13+ / Clang 17+ / MSVC 19.34+ (C++20)
- [vcpkg](https://vcpkg.io/) with: `vulkan`, `glfw3`, `glm`, `stb`, `tinyobjloader`, `tinygltf`
- Vulkan SDK (validation layers + `dxc` for HLSL -> SPIR-V)
- ImGui built from source via `FindImgui.cmake` (FetchContent, v1.92.6) — vcpkg with imgui seems to have some issues.

### Building

```sh
cmake -B build
cmake --build build
```

The CMakeLists auto-detects vcpkg via `$VCPKG_ROOT`, a local `vcpkg/` directory, or `C:/vcpkg`.

## Roadmap

### Tutorial (complete)

- [x] Vertex / index buffers
- [x] Uniform buffers + descriptor sets
- [x] Texture mapping + mipmaps
- [x] Depth buffering
- [x] Model loading (OBJ)
- [x] Multisampling (MSAA)

### Engine work (complete)

- [x] Modular architecture
- [x] Orbit camera with smooth interpolation + free camera mode
- [x] Scene graph with per-object transforms and parented orbital bodies
- [x] GPU picking + hover outline
- [x] ImGui UI (pause, camera mode, planet list with moons, planet info panel)
- [x] Per-object textures
- [x] Planet axial rotation (including retrograde for Venus and Uranus)
- [x] Solar system scene — sun, 8 planets, moons, asteroid belt
- [x] Orbit circle visualization
- [x] Skybox (equirectangular milky way)
- [x] glTF / GLB model loading

### Planned

- [ ] Multi-texture materials (normal map, specular, night lights)
- [ ] Sun glow / bloom
- [ ] Saturn's rings
- [ ] ECS refactor
- [ ] Asset manager

## Credits

- Planet and sun textures from [Solar System Scope](https://www.solarsystemscope.com/textures/) —
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- Skybox and moon textures from [NASA Visible Earth](https://visibleearth.nasa.gov) — public domain.
- "Asteroid low poly" by [pasquill](https://sketchfab.com/pasquill) on Sketchfab
([source](https://sketchfab.com/3d-models/asteroid-low-poly-9a43ef48a70647188576ccb5987b7e64)) —
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- "Painterly Cottage" by [glenatron](https://sketchfab.com/glenatron) on Sketchfab
([source](https://sketchfab.com/3d-models/painterly-cottage-0772aec70d584c60a27000af5f6c1ef4)),
licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Removed ground (since we only use 1
texture per model for now) and scaled up.
- "Viking Room" by [nigelgoh](https://sketchfab.com/nigelgoh) on Sketchfab
([source](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)) —
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).