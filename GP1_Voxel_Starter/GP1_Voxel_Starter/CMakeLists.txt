#==============================================================
#  Graphics Programming 1 (2026-2027)
#  Authors : Matthieu Delaere
#  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
#==============================================================
cmake_minimum_required(VERSION 3.25)
# CMP0111: We use OLD behavior as a workaround for Conan 2 + Ninja Multi-Config.
# Conan's CMakeDeps generates IMPORTED_LOCATION only for the active build_type,
# but Ninja Multi-Config expects all configurations to be defined simultaneously.
# Suppress the deprecation warning temporarily until Conan fixes this issue.
# See: https://github.com/conan-io/conan/issues/12077
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
if(POLICY CMP0111)
    cmake_policy(SET CMP0111 OLD)
endif()
set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
project(GP1_Voxel VERSION 1.0.0 LANGUAGES CXX C)

# --- C++ standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# --- Compiler specific settings where we don't rely on conan ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_AR "${CMAKE_CXX_COMPILER_AR}")
    set(CMAKE_RANLIB "${CMAKE_CXX_COMPILER_RANLIB}")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(UNIX)
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address>)
    endif()
    if(UNIX)
        add_compile_options($<$<CONFIG:Release>:-flto>)
        add_link_options($<$<CONFIG:Release>:-flto>)
    endif()
    if(WIN32)
        # GCC 15 + MinGW: dependency .a files emit strong libstdc++ symbols that
        # conflict with libstdc++.dll.a at -O2/-O3. Allow duplicates to link.
        add_link_options("LINKER:--allow-multiple-definition")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /WX /Zc:preprocessor)
endif()

# --- Globally shared compiler options ---
add_compile_definitions($<$<CONFIG:Debug>:GFX_DEBUG>)
if(WIN32)
    add_compile_definitions(GFX_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(GFX_PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(GFX_PLATFORM_LINUX)
    option(GFX_USE_WAYLAND "Use Wayland instead of X11" OFF)
    if(GFX_USE_WAYLAND)
        add_compile_definitions(GFX_PLATFORM_WAYLAND)
    else()
        add_compile_definitions(GFX_PLATFORM_X11)
    endif()
endif()

# --- Set output directories for all projects ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# --- Find dependencies ---
find_package(spdlog REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

# --- Set files ---
set(PROJECT_SOURCE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/source")
file(GLOB_RECURSE PROJECT_FRAMEWORK_SOURCE_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIRECTORY}/*.h"
    "${PROJECT_SOURCE_DIRECTORY}/*.cpp")

# --- Create project and link accordingly ---
add_executable(${PROJECT_NAME}
    ${PROJECT_FRAMEWORK_SOURCE_FILES})

# --- Include directories ---
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIRECTORY}"
        "${PROJECT_SOURCE_DIRECTORY}/core"
        "${PROJECT_SOURCE_DIRECTORY}/shading"
        "${PROJECT_SOURCE_DIRECTORY}/scene"
        "${PROJECT_SOURCE_DIRECTORY}/renderers")

# --- Link libraries ---
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        SDL3::SDL3)

# --- Copy resources folder (re-copies when any resource file changes) ---
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
set(RESOURCE_STAMP "${CMAKE_BINARY_DIR}/resources.stamp")
add_custom_command(
    OUTPUT "${RESOURCE_STAMP}"
    DEPENDS ${RESOURCE_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
    COMMAND ${CMAKE_COMMAND} -E touch "${RESOURCE_STAMP}"
    COMMENT "Copying resource folder to output directory"
)
add_custom_target(CopyResources DEPENDS "${RESOURCE_STAMP}")
add_dependencies(${PROJECT_NAME} CopyResources)

# --- Stand-alone HitTestVoxelGrid test (no test framework, no SDL/spdlog) ---
# Only pulls in the math, primitives and asset loading code that the scenes need.
set(TEST_PROJECT_NAME HitTestVoxelGridTest)
add_executable(${TEST_PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/hit_test_voxel_grid_test.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/core/asset_loader.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/core/matrix.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/core/vector2.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/core/vector3.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/core/vector4.cpp"
    "${PROJECT_SOURCE_DIRECTORY}/scene/primitives.cpp")
target_include_directories(${TEST_PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCE_DIRECTORY}"
        "${PROJECT_SOURCE_DIRECTORY}/core"
        "${PROJECT_SOURCE_DIRECTORY}/shading"
        "${PROJECT_SOURCE_DIRECTORY}/scene"
        "${PROJECT_SOURCE_DIRECTORY}/renderers")
add_dependencies(${TEST_PROJECT_NAME} CopyResources)

# Run with `ctest`; working directory is the exe folder so resources/ is found.
enable_testing()
add_test(NAME ${TEST_PROJECT_NAME} COMMAND ${TEST_PROJECT_NAME}
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TEST_PROJECT_NAME}>")
