# Vulkan Engine

## Compile
```shell
cmake -S "PROJ_FOLDER" `
  -B "PROJ_FOLDER\cmake-build-debug" `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

## Build
```shell
cmake --build "PROJ_FOLDER\cmake-build-debug" --config Debug
```

## Run
```shell
& "PROJ_FOLDER\cmake-build-debug\vulkan_tutorial_app.exe"
```