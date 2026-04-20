# Vulkan Engine

A Vulkan rendering engine built while following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/),
refactored from a single-file application into a modular engine structure.

## Current Progress

Tutorial progress: **swap chain recreation** (end of the "Drawing a triangle" chapter).

The application opens a window and renders a hardcoded triangle using dynamic rendering (no render passes). Window
resize and minimize are handled correctly.

## Project Structure

```
src/
  main.cpp                ← Entry point; constructs Engine and runs it
  core/
    Config.hpp            ← EngineConfig struct (window size, shader paths, etc.)
    Engine.hpp/.cpp       ← Top-level composer; owns all subsystems and runs the main loop
    Window.hpp/.cpp       ← GLFW window wrapper; creates the VkSurfaceKHR
  vk/
    Instance.hpp/.cpp     ← Vulkan instance + debug messenger + validation layers
    Device.hpp/.cpp       ← Physical device selection + logical device + queue
    SwapChain.hpp/.cpp    ← Swapchain + image views + recreation logic
    Pipeline.hpp/.cpp     ← Graphics pipeline + pipeline layout + shader module loading
    Renderer.hpp/.cpp     ← Command pool/buffers, sync objects, per-frame draw loop
  io/
    FileIO.hpp/.cpp       ← Generic binary file reading (used for SPIR-V)
shaders/
  shader.slang            ← Source shader (Slang language, compiled to SPIR-V)
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
  ├── Pipeline ────────── needs Device + swapchain format
  └── Renderer ────────── needs Device + SwapChain + Pipeline
```

Construction flows top-down; destruction happens in reverse via RAII. Member declaration order in `Engine.hpp`
determines both.

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

`Renderer::drawFrame` handles fences, semaphores, command buffer recording, submission, and presentation. It calls
`swapChain.recreate()` internally when the swapchain becomes out-of-date or when the caller signals a resize.

## Build

### Dependencies

- CMake ≥ 3.29
- C++20-capable compiler (GCC 13+, Clang 17+, MSVC 19.34+)
- [vcpkg](https://vcpkg.io/) with:
    - `vulkan`
    - `glfw3`
    - `glm`
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

### Running

```sh
./build/vulkan_tutorial_app
```

The executable expects `shaders/` to be adjacent to it (the CMake post-build step handles this).

## Vulkan-Hpp Notes

The project uses `vulkan_raii.hpp` (header-only path) rather than the C++20 `vulkan_hpp` module. This requires:

- `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` in exactly one TU (`Instance.cpp`).
- Explicit `VULKAN_HPP_DEFAULT_DISPATCHER.init()` calls in `Instance::createInstance` (pre-instance) and after the instance is created (instance-level functions). `Device` initializes device-level dispatch in its constructor.

These are set up in `CMakeLists.txt`:

```cmake
target_compile_definitions(vulkan_tutorial_app PRIVATE
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
```


## Required GPU Features

The device selection in `Device::isDeviceSuitable` requires:

- Vulkan 1.3
- Graphics queue family with present support for the surface
- `VK_KHR_swapchain` extension
- Features: `shaderDrawParameters`, `dynamicRendering`, `synchronization2`, `extendedDynamicState`

Most desktop GPUs from the last few years meet these requirements. Integrated graphics may need driver updates.

## Roadmap

Following the Vulkan Tutorial, the next topics are:

- [ ] Vertex buffers
- [ ] Index buffers
- [ ] Uniform buffers + descriptor sets
- [ ] Texture mapping
- [ ] Depth buffering
- [ ] Model loading
- [ ] Mipmaps
- [ ] Multisampling

Each chapter will likely require extending existing classes (e.g., `Pipeline` grows to accept descriptor set layouts) or adding new ones (e.g., a `Buffer` class wrapping `vk::raii::Buffer` + `vk::raii::DeviceMemory`).
