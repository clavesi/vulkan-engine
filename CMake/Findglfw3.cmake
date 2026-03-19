# Findglfw3.cmake
#
# Locate GLFW3 (glfw) library and headers.
# Sets GLFW3_FOUND, GLFW3_INCLUDE_DIRS, GLFW3_LIBRARIES and creates an
# imported target GLFW::GLFW (and glfw / glfw3::glfw alias if not present).

include(FindPackageHandleStandardArgs)

find_path(GLFW3_INCLUDE_DIR
  NAMES GLFW/glfw3.h glfw3.h
  HINTS
    $ENV{VCPKG_ROOT}/installed
    $ENV{VCPKG_ROOT}
    $ENV{VULKAN_SDK}/include
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    /usr/include
    /usr/local/include
)

find_library(GLFW3_LIBRARY
  NAMES glfw3 glfw
  HINTS
    $ENV{VCPKG_ROOT}/installed
    $ENV{VCPKG_ROOT}
    $ENV{VULKAN_SDK}/lib
    ${CMAKE_SOURCE_DIR}/external/glfw/lib
    /usr/lib
    /usr/local/lib
)

if(GLFW3_INCLUDE_DIR AND GLFW3_LIBRARY)
  set(GLFW3_FOUND TRUE)
else()
  # Try pkg-config as a fallback
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GLFW QUIET glfw3)
    if(PC_GLFW_FOUND)
      set(GLFW3_FOUND TRUE)
      set(GLFW3_INCLUDE_DIR ${PC_GLFW_INCLUDEDIR})
      # pkg-config may provide libs as a list
      if(PC_GLFW_LIBDIR)
        find_library(GLFW3_LIBRARY NAMES glfw3 HINTS ${PC_GLFW_LIBDIR})
      endif()
    endif()
  endif()
endif()

if(GLFW3_FOUND)
  if(NOT DEFINED GLFW3_INCLUDE_DIR AND DEFINED PC_GLFW_INCLUDEDIR)
    set(GLFW3_INCLUDE_DIR ${PC_GLFW_INCLUDEDIR})
  endif()
  if(NOT DEFINED GLFW3_LIBRARY AND DEFINED PC_GLFW_LIBRARIES)
    # PC_GLFW_LIBRARIES may contain full linker flags; try to extract library file
    set(GLFW3_LIBRARY ${PC_GLFW_LIBRARIES})
  endif()

  set(GLFW3_INCLUDE_DIRS ${GLFW3_INCLUDE_DIR})
  set(GLFW3_LIBRARIES ${GLFW3_LIBRARY})

  # Create imported target(s) to match common package names
  if(NOT TARGET GLFW::GLFW)
    add_library(GLFW::GLFW UNKNOWN IMPORTED)
    set_target_properties(GLFW::GLFW PROPERTIES
      IMPORTED_LOCATION "${GLFW3_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${GLFW3_INCLUDE_DIRS}"
    )
  endif()

  # Provide compatibility aliases
  if(NOT TARGET glfw)
    add_library(glfw UNKNOWN IMPORTED)
    set_target_properties(glfw PROPERTIES
      IMPORTED_LOCATION "${GLFW3_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${GLFW3_INCLUDE_DIRS}"
    )
  endif()
  if(NOT TARGET glfw3::glfw)
    add_library(glfw3::glfw ALIAS glfw)
  endif()

else()
  set(GLFW3_FOUND FALSE)
endif()

if(NOT GLFW3_FOUND)
  # If GLFW is not available on the system, fetch and build it using FetchContent.
  include(FetchContent)

  message(STATUS "glfw3 not found on system — Fetching GLFW via FetchContent...")

  # Configure GLFW build options to avoid extra build time and installation steps.
  # These cache variables are set before the GLFW project is added so they take effect.
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW docs" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW" FORCE)

  # Prefer building as a static lib to avoid exporting/installation complexities
  # but allow the user's global BUILD_SHARED_LIBS to override if explicitly set.
  if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
  endif()

  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4 # stable tag; adjust if you need a different version
  )

  FetchContent_MakeAvailable(glfw)

  # After building, the GLFW target is typically named 'glfw' (no namespace).
  if(TARGET glfw)
    # Provide a namespaced alias expected by some Find/config packages
    if(NOT TARGET GLFW::GLFW)
      add_library(GLFW::GLFW ALIAS glfw)
    endif()
    if(NOT TARGET glfw3::glfw)
      add_library(glfw3::glfw ALIAS glfw)
    endif()

    # Expose include directory and library variables for compatibility
    get_target_property(_glfw_include_dirs glfw INTERFACE_INCLUDE_DIRECTORIES)
    if(_glfw_include_dirs)
      set(GLFW3_INCLUDE_DIRS ${_glfw_include_dirs})
    endif()
    set(GLFW3_LIBRARIES $<TARGET_FILE:glfw>)
    set(GLFW3_FOUND TRUE)
  else()
    message(WARNING "FetchContent built GLFW but no 'glfw' target was found — consumers expecting GLFW targets may fail.")
  endif()
endif()

message(STATUS "glfw3 found: ${GLFW3_FOUND} (include: ${GLFW3_INCLUDE_DIR}, lib: ${GLFW3_LIBRARY})")

