cmake_minimum_required(VERSION 3.29)

# Minimal CMakeLists for the engine: auto-detect vcpkg, prefer config packages,
# set C++20, expose repo CMake/ Find modules, find core deps and build App.cpp.

# Detect a vcpkg toolchain automatically so vcpkg-installed packages are
# available without requiring the user to manually set the toolchain in CLion.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root $ENV{VCPKG_ROOT})
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg")
    set(_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
  elseif(EXISTS "C:/vcpkg")
    set(_vcpkg_root "C:/vcpkg")
  endif()

  if(DEFINED _vcpkg_root)
    set(_vcpkg_toolchain "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
    if(EXISTS "${_vcpkg_toolchain}")
      set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_toolchain}" CACHE FILEPATH "Vcpkg toolchain file" FORCE)
      message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    endif()
  endif()
endif()

project(VulkanTutorial LANGUAGES CXX)

# Default to C++20
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to use" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add repo-provided Find modules to the module path so they are available
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

# Prefer config-mode packages (vcpkg) by default; repo Find modules remain available
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Core dependencies needed for App.cpp
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Minimal executable
add_executable(vulkan_tutorial_app App.cpp)

# Link Vulkan first
if(TARGET Vulkan::Vulkan)
  target_link_libraries(vulkan_tutorial_app PRIVATE Vulkan::Vulkan)
elseif(DEFINED Vulkan_LIBRARY)
  target_link_libraries(vulkan_tutorial_app PRIVATE ${Vulkan_LIBRARY})
endif()

# Link GLFW (support common target names)
if(TARGET glfw)
  target_link_libraries(vulkan_tutorial_app PRIVATE glfw)
elseif(TARGET glfw3::glfw)
  target_link_libraries(vulkan_tutorial_app PRIVATE glfw3::glfw)
elseif(TARGET GLFW::GLFW)
  target_link_libraries(vulkan_tutorial_app PRIVATE GLFW::GLFW)
endif()

# Link glm if available as a target
if(TARGET glm::glm)
  target_link_libraries(vulkan_tutorial_app PRIVATE glm::glm)
endif()

# Ensure C++20 is set on the target
set_target_properties(vulkan_tutorial_app PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

# Copy assets next to the executable if the assets directory exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
  add_custom_command(TARGET vulkan_tutorial_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:vulkan_tutorial_app>/assets
  )
endif()

