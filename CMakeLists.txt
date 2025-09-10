cmake_minimum_required(VERSION 3.20)
project(Gravix VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(GravixConfig)

# Build options
option(ENGINE_BUILD_VULKAN "Build with Vulkan support" ON)
option(ENGINE_BUILD_D3D12 "Build with D3D12 support" ON)
option(ENGINE_BUILD_AUDIO "Build with Audio support" ON)
option(ENGINE_BUILD_PHYSICS "Build with Physics support" ON)
option(ENGINE_BUILD_EDITOR "Build Editor" ON)
option(ENGINE_BUILD_TESTS "Build Tests" OFF)

# Fetch all dependencies
include(FetchAllDependencies)

# Platform detection
if(WIN32)
    set(ENGINE_PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(ENGINE_PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(ENGINE_PLATFORM_MACOS TRUE)
endif()

# Global compile definitions
add_compile_definitions(
    $<$<CONFIG:Debug>:ENGINE_DEBUG>
    $<$<CONFIG:Release>:ENGINE_RELEASE>
    $<$<BOOL:${ENGINE_PLATFORM_WINDOWS}>:ENGINE_PLATFORM_WINDOWS>
    $<$<BOOL:${ENGINE_PLATFORM_LINUX}>:ENGINE_PLATFORM_LINUX>
    $<$<BOOL:${ENGINE_PLATFORM_MACOS}>:ENGINE_PLATFORM_MACOS>
)

# Add engine modules
add_subdirectory(Gravix)

if(ENGINE_BUILD_EDITOR)
    add_subdirectory(Orbit)
endif()

if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()