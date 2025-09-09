cmake_minimum_required(VERSION 3.16)
project(GravixEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# === Supported Platforms ===
set(SUPPORTED_PLATFORMS WINDOWS LINUX MAC)

# === Default to Windows if TARGET_PLATFORM is not defined ===
if (NOT DEFINED TARGET_PLATFORM)
    message(STATUS "TARGET_PLATFORM not specified — defaulting to WINDOWS")
    set(TARGET_PLATFORM "WINDOWS")
endif()

string(TOUPPER "${TARGET_PLATFORM}" TARGET_PLATFORM_UPPER)

if (NOT TARGET_PLATFORM_UPPER IN_LIST SUPPORTED_PLATFORMS)
    message(FATAL_ERROR "Unsupported platform: ${TARGET_PLATFORM_UPPER}")
endif()

# === Platform-specific macro ===
if (TARGET_PLATFORM_UPPER STREQUAL "WINDOWS")
    add_compile_definitions(PLATFORM_WINDOWS)
elseif (TARGET_PLATFORM_UPPER STREQUAL "LINUX")
    add_compile_definitions(PLATFORM_LINUX)
elseif (TARGET_PLATFORM_UPPER STREQUAL "MAC")
    add_compile_definitions(PLATFORM_MAC)
endif()

# === Final Build Define ===
option(PRODUCTION_BUILD "Build for final game distribution" OFF)

if (PRODUCTION_BUILD)
    add_compile_definitions(PRODUCTION_BUILD)
endif()

# === Add engine modules ===
add_subdirectory(Gravis-Windows)
add_subdirectory(Gravis-Vulkan)
add_subdirectory(Gravis)
add_subdirectory(Orbit)
