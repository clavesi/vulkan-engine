# Vulkan Engine

A Vulkan rendering engine built while following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/),
refactored from a single-file application into a modular engine structure.

## Current Progress

Tutorial progress: **texture mapping** (end of the "Texture mapping" chapter).

The application opens a window and renders a textured 3D square that spins in perspective view. It uses dynamic
rendering (no render passes), index buffers, per-frame uniform buffers for MVP matrices, and a combined image sampler
descriptor for the texture. Window resize and minimize are handled correctly.

## Project Structure

```
src/
  main.cpp                    ← Entry point; constructs Engine and runs it
  core/
    Config.hpp                ← EngineConfig struct (window size, shader paths, etc.)
    Engine.hpp/.cpp           ← Top-level composer; owns all subsystems and runs the main loop
    Window.hpp/.cpp           ← GLFW window wrapper; creates the VkSurfaceKHR
    Vertex.hpp                ← Vertex struct (pos, color, texCoord) + binding/attribute descriptions
    UniformBufferObject.hpp   ← MVP matrix struct shared with the shader
  vk/
    Instance.hpp/.cpp         ← Vulkan instance + debug messenger + validation layers
    Device.hpp/.cpp           ← Physical device selection + logical device + queue + transient pool
    SwapChain.hpp/.cpp        ← Swapchain + image views + recreation logic
    Pipeline.hpp/.cpp         ← Graphics pipeline + descriptor set layout + shader module loading
    Buffer.hpp/.cpp           ← vk::raii::Buffer + DeviceMemory wrapper; upload + staging helpers
    Image.hpp/.cpp            ← vk::raii::Image + DeviceMemory wrapper; layout transitions, view creation
    Sampler.hpp/.cpp          ← vk::raii::Sampler wrapper
    Renderer.hpp/.cpp         ← Command pool/buffers, sync, descriptors, per-frame draw loop
  io/
    FileIO.hpp/.cpp           ← Generic binary file reading (used for SPIR-V)
shaders/
  shader.slang                ← Source shader (Slang language, compiled to SPIR-V)
textures/
  texture.jpg                 ← Sample texture loaded by the renderer
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
  ├── SwapChain ───────── needs Device + Window + Surface
  ├── Pipeline ────────── needs Device + swapchain format + descriptor bindings
  └── Renderer ────────── needs Device + SwapChain + Pipeline
                          (owns Buffers, Image, Sampler internally)
```

Construction flows top-down; destruction happens in reverse via RAII. Member declaration order in `Engine.hpp`
determines both.

### Resource wrappers

Three thin classes wrap the Vulkan resource primitives:

- **`Buffer`** — pairs `vk::raii::Buffer` + `vk::raii::DeviceMemory`. Supports direct upload (`uploadData`),
  staged upload to device-local memory (`uploadViaStaging`), and persistent mapping (`mapPersistent`) for
  per-frame uniform writes.
- **`Image`** — pairs `vk::raii::Image` + `vk::raii::DeviceMemory`. Supports layout transitions
  (`transitionLayout`), copying from a staging buffer (`copyFromBuffer`), and image view creation (`createView`).
- **`Sampler`** — wraps `vk::raii::Sampler` with sensible defaults (linear filtering, anisotropy, repeat addressing).

These wrappers compose: `Renderer` holds a vertex `Buffer`, an index `Buffer`, a vector of uniform `Buffer`s
(one per frame in flight), a texture `Image`, an `ImageView`, and a `Sampler`.

### Design principles

- **RAII everywhere.** All Vulkan handles are `vk::raii::*` types. No manual `vkDestroy*` calls. Destruction order is
  controlled by member declaration order.
- **Dependencies passed by `const&` in constructors.** Subsystems store references to their dependencies. This makes
  lifetimes explicit and surfaces ordering bugs at compile time.
- **No singletons or globals.** The only global state is the Vulkan-Hpp dynamic dispatch loader, which lives in
  `Instance.cpp`.
- **Classes don't know about things above them.** `Renderer` doesn't know about `Window`; `SwapChain` doesn't know about
  `Engine`. This keeps the graph acyclic.
- **Configuration lives in `EngineConfig`.** Window size, shader paths, and future engine-wide settings go there, not
  scattered as constants.

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
- Vulkan SDK (for `slangc` shader compiler and validation layers)

### Building

The CMakeLists auto-detects vcpkg via `$VCPKG_ROOT`, a local `vcpkg/` directory, or `C:/vcpkg`.

```sh
cmake -B build
cmake --build build
```

Or open the project in CLion / Visual Studio and build from there.

### Shaders

Slang shaders in `shaders/*.slang` are compiled to SPIR-V at build time via `slangc` (from the Vulkan SDK). The
resulting `.spv` files are copied next to the executable.

If `slangc` is not found, the `shaders/` directory is copied as-is — you'll need to pre-compile your shaders manually in
that case.

### Textures

The `textures/` directory is copied next to the executable at build time. Loaded at runtime via `stb_image`. Drop in
any JPG/PNG named `texture.jpg` to swap the displayed texture.

### Running

```sh
./build/vulkan_tutorial_app
```

The executable expects `shaders/` and `textures/` to be adjacent to it (the CMake post-build steps handle this).

## Vulkan-Hpp Notes

The project uses `vulkan_raii.hpp` (header-only path) rather than the C++20 `vulkan_hpp` module. This requires:

- `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` in exactly one TU (`Instance.cpp`).
- Explicit `VULKAN_HPP_DEFAULT_DISPATCHER.init()` calls in `Instance::createInstance` (pre-instance) and after the instance is created (instance-level functions). `Device` initializes device-level dispatch in its constructor.

These are set up in `CMakeLists.txt`:

```cmake
target_compile_definitions(vulkan_tutorial_app PRIVATE
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)
```

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
- [ ] Depth buffering
- [ ] Model loading
- [ ] Mipmaps
- [ ] Multisampling

Each chapter typically extends existing classes (e.g., `Pipeline` grew to accept `PipelineSpec` with vertex inputs
and descriptor bindings) or adds new ones (e.g., `Buffer`, `Image`, `Sampler` were each introduced when their
respective primitives were first needed).