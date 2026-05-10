# FindImgui.cmake
# Finds Dear ImGui, or fetches and builds it from source if not found.
#
# This will define the following imported targets:
#   imgui::imgui

# Try vcpkg / system install first
find_path(imgui_INCLUDE_DIR
        NAMES imgui.h
        PATHS
        ${CMAKE_PREFIX_PATH}
        /usr/include
        /usr/local/include
        $ENV{VULKAN_SDK}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../third_party
        PATH_SUFFIXES imgui
)

if(NOT imgui_INCLUDE_DIR)
    include(FetchContent)

    message(STATUS "imgui not found locally, fetching from GitHub...")

    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG        v1.92.6
            GIT_SHALLOW    TRUE
    )

    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()

    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    set(imgui_INCLUDE_DIR
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )

    # Build as a static library compiled with whatever compiler the project uses
    if(NOT TARGET imgui_compiled)
        add_library(imgui_compiled STATIC
                ${imgui_SOURCE_DIR}/imgui.cpp
                ${imgui_SOURCE_DIR}/imgui_draw.cpp
                ${imgui_SOURCE_DIR}/imgui_tables.cpp
                ${imgui_SOURCE_DIR}/imgui_widgets.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        )
        target_include_directories(imgui_compiled PUBLIC
                ${imgui_SOURCE_DIR}
                ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui_compiled PUBLIC
                glfw
                Vulkan::Vulkan
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Imgui
        REQUIRED_VARS imgui_INCLUDE_DIR
)

if(Imgui_FOUND)
    if(NOT TARGET imgui::imgui)
        if(TARGET imgui_compiled)
            # Built from source — alias it
            add_library(imgui::imgui ALIAS imgui_compiled)
        else()
            # Found via vcpkg/system — but won't link on MinGW, so warn
            message(WARNING "imgui found as precompiled lib — may not link with MinGW. Consider removing vcpkg imgui.")
            add_library(imgui::imgui INTERFACE IMPORTED)
            set_target_properties(imgui::imgui PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${imgui_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()

mark_as_advanced(imgui_INCLUDE_DIR)