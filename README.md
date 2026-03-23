# Vulkan Engine

## Compile
```shell
cmake -S . -B cmake-build-debug -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DSLANGC_EXECUTABLE="C:/VulkanSDK/1.4.341.1/bin/slangc.exe" `
  -DPREFER_CONFIG_PACKAGES=ON
```

## Build
Invoke only shader step
```shell
cmake --build cmake-build-debug --target shaders --config Debug
```

Build the entire project
```shell
cmake --build cmake-build-debug --config Debug
```

## Run
```shell
.\cmake-build-debug\vulkan_tutorial_app.exe
```