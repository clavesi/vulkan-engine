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

# Shader compilation with Slang (optional, but preferred when slangc is available)
# Use a relative glob pattern so paths like "[0]Vulkan" do not interfere with matching.
file(GLOB SLANG_SHADER_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "shaders/*.slang")
if(NOT SLANG_SHADER_SOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang")
  set(SLANG_SHADER_SOURCES "shaders/shader.slang")
endif()
find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/bin")

if(SLANGC_EXECUTABLE AND SLANG_SHADER_SOURCES)
  # Place generated SPIR-V files in the build tree.
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

  foreach(SHADER_REL ${SLANG_SHADER_SOURCES})
    set(SHADER "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_REL}")
    get_filename_component(SHADER_NAME "${SHADER_REL}" NAME)
    get_filename_component(SHADER_NAME_WE "${SHADER_REL}" NAME_WE)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME_WE}.spv"
      COMMAND ${SLANGC_EXECUTABLE}
              "${SHADER}"
              -target spirv
              -profile spirv_1_4
              -emit-spirv-directly
              -fvk-use-entrypoint-name
              -entry vertMain
              -entry fragMain
              -o "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME_WE}.spv"
      DEPENDS "${SHADER}"
      COMMENT "Compiling Slang shader ${SHADER_NAME}"
      VERBATIM
    )
    list(APPEND SHADER_SPVS "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME_WE}.spv")
  endforeach()

  add_custom_target(shaders DEPENDS ${SHADER_SPVS})
else()
  if(NOT SLANGC_EXECUTABLE)
    message(STATUS "slangc not found. Falling back to source shaders directory copy.")
  elseif(NOT SLANG_SHADER_SOURCES)
    message(STATUS "No .slang shader files found under ${CMAKE_CURRENT_SOURCE_DIR}/shaders")
  endif()
  add_custom_target(shaders)
endif()

# Minimal executable
add_executable(vulkan_tutorial_app App.cpp)
add_dependencies(vulkan_tutorial_app shaders)

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
set_target_properties(vulkan_tutorial_app PROPERTIES CXX_STANDARD 20)

# Compatibility flags for Vulkan-Hpp (allow aggregate-style initialization of vk:: structs
# and enable the dynamic dispatch loader).
target_compile_definitions(vulkan_tutorial_app PRIVATE
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# Copy assets next to the executable if the assets directory exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
  add_custom_command(TARGET vulkan_tutorial_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:vulkan_tutorial_app>/assets
  )
endif()

# Copy shaders next to the executable.
if(SLANGC_EXECUTABLE)
  add_custom_command(TARGET vulkan_tutorial_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders $<TARGET_FILE_DIR:vulkan_tutorial_app>/shaders
  )
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
  add_custom_command(TARGET vulkan_tutorial_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:vulkan_tutorial_app>/shaders
  )
endif()

