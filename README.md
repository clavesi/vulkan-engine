# Vulkan Engine

A Vulkan rendering engine built while following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/),
extensively refactored into a modular engine and extended well beyond the tutorial into original engine work.

## Current State

The tutorial phase is complete. The engine has moved on to original architecture work in pursuit of a simple solar
system demo scene.

## Project Structure

```
src/engine/
  core/       — Camera, Input, Scene, SceneObject, Mesh, Transform, OrbitalBody, config
  vk/         — Vulkan wrappers: Instance, Device, SwapChain, Pipeline, Renderer, Buffer, Image, Texture
  io/         — File reading, OBJ model loading
  ui/         — ImGui integration
  app/        — Reserved for solar system specific code
shaders/      — HLSL source + compiled SPIR-V
textures/
  solar/      — 2K planet texture maps (NASA/public domain)
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

### GPU picking

A picking pass runs every frame before the main pass, rendering each object's index as a `uint` into an
`R32_UINT` image. The pixel under the cursor is copied to a host-visible readback buffer and read back
the following frame after the fence. Cursor coordinates are multiplied by `contentScale` for HiDPI displays.

### Camera

Orbit camera with smooth exponential interpolation on yaw, pitch, distance, and target. Double-clicking
an object locks the camera to follow it. ESC resets to the default view.

## Controls

| Input               | Action                    |
|---------------------|---------------------------|
| Right mouse drag    | Orbit camera              |
| Scroll              | Zoom                      |
| Double-click object | Follow object             |
| ESC                 | Reset camera              |
| SPACE               | Pause / resume simulation |

## Build

### Dependencies

- CMake >= 3.29
- GCC 13+ / Clang 17+ / MSVC 19.34+ (C++20)
- [vcpkg](https://vcpkg.io/) with: `vulkan`, `glfw3`, `glm`, `stb`, `tinyobjloader`
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
- [x] Orbit camera with smooth interpolation
- [x] Scene graph with per-object transforms
- [x] GPU picking + hover outline
- [x] ImGui UI
- [x] Per-object textures
- [x] Solar system scene (sun + orbiting planets)

### Planned

- [ ] glTF model loading
- [ ] Real solar system data (scales, orbital periods, axial tilt)
- [ ] Moons (parented orbital bodies)
- [ ] Sun glow / bloom
- [ ] ECS refactor

## Credits

- Planet textures from [Solar System Scope](https://www.solarsystemscope.com/textures/) — [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- "Painterly Cottage" by [glenatron](https://sketchfab.com/glenatron) on Sketchfab
  ([source](https://sketchfab.com/3d-models/painterly-cottage-0772aec70d584c60a27000af5f6c1ef4)),
  licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Removed ground (since we only use 1
  texture per model for now) and scaled up.
- "Viking Room" by [nigelgoh](https://sketchfab.com/nigelgoh) on Sketchfab
  ([source](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)),
  licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Used as the sample model in the
  official Vulkan Tutorial.
