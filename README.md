# Vulkan Engine

A Vulkan rendering engine built while following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/),
refactored from a single-file application into a modular engine structure.

## Current Progress

Tutorial progress: **generating mipmaps** (end of the "Generating Mipmaps" chapter).

The application opens a window and renders a textured 3D model loaded from an OBJ file, with depth buffering,
runtime-generated mipmaps, and perspective view. It uses dynamic rendering (no render passes), index buffers,
per-frame uniform buffers for MVP matrices, and a combined image sampler descriptor for the texture. Window resize
and minimize are handled correctly.

## Project Structure

```
src/
  main.cpp                  ← Entry point; constructs Engine and runs it
  core/
    Config.h                ← EngineConfig struct (window size, shader paths, model/texture paths, etc.)
    Engine.h/.cpp           ← Top-level composer; owns all subsystems and runs the main loop
    Window.h/.cpp           ← GLFW window wrapper; creates the VkSurfaceKHR
    Vertex.h                ← Vertex struct (pos, color, texCoord) + binding/attribute descriptions + hash
    UniformBufferObject.h   ← MVP matrix struct shared with the shader
    Mesh.h/.cpp             ← Owns vertex + index Buffer and index count
  vk/
    Instance.h/.cpp         ← Vulkan instance + debug messenger + validation layers
    Device.h/.cpp           ← Physical device selection + logical device + queue + transient pool + format helpers
    SwapChain.h/.cpp        ← Swapchain + image views + depth resources + recreation logic
    Pipeline.h/.cpp         ← Graphics pipeline + descriptor set layout + shader module loading
    Buffer.h/.cpp           ← vk::raii::Buffer + DeviceMemory wrapper; upload + staging helpers
    Image.h/.cpp            ← vk::raii::Image + DeviceMemory wrapper; layout transitions, view creation, mipmap generation
    Sampler.h/.cpp          ← vk::raii::Sampler wrapper with configurable maxLod for mipmapping
    Renderer.h/.cpp         ← Command pool/buffers, sync, descriptors, per-frame draw loop
  io/
    FileIO.h/.cpp           ← Generic binary file reading (used for SPIR-V)
    ModelLoader.h/.cpp      ← OBJ loading via tinyobjloader, with vertex deduplication
shaders/
  shader.slang              ← Source shader (Slang language, compiled to SPIR-V)
textures/
  *.png                     ← Texture loaded by the renderer (path configured via EngineConfig)
models/
  *.obj                     ← Model loaded by the renderer (path configured via EngineConfig)
CMakeLists.txt
```

## Architecture

The engine is organized as a composition of single-responsibility classes, owned by `Engine`.

### Dependency graph

```
Engine
  ├── Window
  ├── Instance
  ├── SurfaceKHR (raw, from Window + Instance)
  ├── Device ──────────── needs Instance + Surface
  ├── SwapChain ───────── needs Device + Window + Surface; owns depth resources
  ├── Pipeline ────────── needs Device + color/depth formats + descriptor bindings
  └── Renderer ────────── needs Device + SwapChain + Pipeline + EngineConfig
                          (owns Mesh, Image, Sampler internally)
```

Construction flows top-down; destruction happens in reverse via RAII. Member declaration order in `Engine.h`
determines both.

### Resource wrappers

Three thin classes wrap the Vulkan resource primitives:

- **`Buffer`** — pairs `vk::raii::Buffer` + `vk::raii::DeviceMemory`. Supports direct upload (`uploadData`),
  staged upload to device-local memory (`uploadViaStaging`), and persistent mapping (`mapPersistent`) for
  per-frame uniform writes.
- **`Image`** — pairs `vk::raii::Image` + `vk::raii::DeviceMemory`. Supports layout transitions
  (`transitionLayout`), copying from a staging buffer (`copyFromBuffer`), image view creation (`createView`),
  and runtime mipmap generation via `vkCmdBlitImage` (`generateMipmaps`). Mip level count is set at construction.
- **`Sampler`** — wraps `vk::raii::Sampler` with sensible defaults (linear filtering, anisotropy, repeat addressing,
  linear mipmap interpolation). Takes `maxLod` at construction to control the active mip range.

A `Mesh` class composes two `Buffer`s (vertex + index) plus an index count, and is the unit `Renderer` works with for
geometry. Renderer holds a `Mesh`, a vector of uniform `Buffer`s (one per frame in flight), a texture `Image`, an
`ImageView`, and a `Sampler`.

### Design principles

- **RAII everywhere.** All Vulkan handles are `vk::raii::*` types. No manual `vkDestroy*` calls. Destruction order is
  controlled by member declaration order.
- **Dependencies passed by `const&` in constructors.** Subsystems store references to their dependencies. This makes
  lifetimes explicit and surfaces ordering bugs at compile time.
- **No singletons or globals.** The only global state is the Vulkan-Hpp dynamic dispatch loader, which lives in
  `Instance.cpp`.
- **Classes don't know about things above them.** `Renderer` doesn't know about `Window`; `SwapChain` doesn't know about
  `Engine`. This keeps the graph acyclic.
- **Configuration lives in `EngineConfig`.** Window size, shader paths, model/texture paths, and future engine-wide
  settings go there, not scattered as constants.

### Frame loop

`Engine::mainLoop` is the simplest possible driver:

```cpp
while (!window.shouldClose()) {
    window.pollEvents();
    const bool resized = window.wasResized();
    if (resized) window.resetResizedFlag();
    renderer.drawFrame(resized);
}
device.waitIdle();
```

`Renderer::drawFrame` handles fences, semaphores, command buffer recording, submission, and presentation. It updates
the per-frame uniform buffer with new MVP matrices, binds the descriptor set (UBO + texture sampler), and calls
`swapChain.recreate()` internally when the swapchain becomes out-of-date or when the caller signals a resize.

## Build

### Dependencies

- CMake ≥ 3.29
- C++20-capable compiler (GCC 13+, Clang 17+, MSVC 19.34+)
- [vcpkg](https://vcpkg.io/) with:
    - `vulkan`
    - `glfw3`
    - `glm`
    - `stb` (for image loading)
    - `tinyobjloader` (for model loading)
- Vulkan SDK (for `slangc` shader compiler and validation layers)

### Building

The CMakeLists auto-detects vcpkg via `$VCPKG_ROOT`, a local `vcpkg/` directory, or `C:/vcpkg`.

```sh
cmake -B build
cmake --build build
```

Or open the project in CLion / Visual Studio and build from there.

**Note:** Model loading benefits significantly from optimization. Build in Release mode (`-DCMAKE_BUILD_TYPE=Release`)
or model load times will be noticeably slow due to vertex deduplication running through a hash map.

### Shaders

Slang shaders in `shaders/*.slang` are compiled to SPIR-V at build time via `slangc` (from the Vulkan SDK). The
resulting `.spv` files are copied next to the executable.

If `slangc` is not found, the `shaders/` directory is copied as-is — you'll need to pre-compile your shaders manually in
that case.

### Textures and models

The `textures/` and `models/` directories are copied next to the executable at build time. The specific texture and
model loaded at runtime are configured via `EngineConfig::texturePath` and `EngineConfig::modelPath`.

Mipmaps are generated at load time from the source texture via `vkCmdBlitImage`. This requires the texture's format
to support `optimalTilingFeatures.eSampledImageFilterLinear` — the engine throws if it doesn't. (For real-world use,
mipmaps are typically pre-baked into the texture file rather than generated at runtime.)

### Running

```sh
./build/vlk_engine
```

The executable expects `shaders/`, `textures/`, and `models/` to be adjacent to it (the CMake post-build steps
handle this).

## Vulkan-Hpp Notes

The project uses `vulkan_raii.hpp` (header-only path) rather than the C++20 `vulkan_hpp` module. This requires:

- `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` in exactly one TU (`Instance.cpp`).
- Explicit `VULKAN_HPP_DEFAULT_DISPATCHER.init()` calls in `Instance::createInstance` (pre-instance) and after the
  instance is created (instance-level functions). `Device` initializes device-level dispatch in its constructor.

These are set up in `CMakeLists.txt`:

```cmake
target_compile_definitions(vlk_engine PRIVATE
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
```

Out-of-date swapchain errors from `presentKHR` are caught explicitly via `try/catch (vk::OutOfDateKHRError&)` in
`Renderer::drawFrame` rather than being suppressed by a compile-time define.

## Required GPU Features

The device selection in `Device::isDeviceSuitable` requires:

- Vulkan 1.3
- Graphics queue family with present support for the surface
- `VK_KHR_swapchain` extension
- Features: `samplerAnisotropy`, `shaderDrawParameters`, `dynamicRendering`, `synchronization2`, `extendedDynamicState`

Most desktop GPUs from the last few years meet these requirements. Integrated graphics may need driver updates.

## Roadmap

Following the Vulkan Tutorial, the next topics are:

- [x] Vertex buffers
- [x] Index buffers
- [x] Uniform buffers + descriptor sets
- [x] Texture mapping
- [x] Depth buffering
- [x] Model loading
- [x] Mipmaps
- [ ] Multisampling

Each chapter typically extends existing classes (e.g., `Pipeline` grew to accept `PipelineSpec` with vertex inputs
and descriptor bindings; `Image` grew to handle multiple mip levels and runtime mipmap generation) or adds new ones
(e.g., `Buffer`, `Image`, `Sampler`, `Mesh` were each introduced when their respective primitives were first needed).

## Credits

Sample model used during development:

- "Painterly Cottage" by [glenatron](https://sketchfab.com/glenatron) on Sketchfab
  ([source](https://sketchfab.com/3d-models/painterly-cottage-0772aec70d584c60a27000af5f6c1ef4)),
  licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Removed ground (since we only use 1
  texture per model for now) and scaled up.
- "Viking Room" by [nigelgoh](https://sketchfab.com/nigelgoh) on Sketchfab
  ([source](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)),
  licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Used as the sample model in the
  official Vulkan Tutorial.

