cmake_minimum_required(VERSION 3.20)
project(GravixEngine VERSION 1.0.0 LANGUAGES CXX C)

# ============================================================================
# Build Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Options
# ============================================================================

option(GRAVIX_USE_VULKAN "Enable Vulkan renderer" ON)
option(GRAVIX_BUILD_EDITOR "Build Orbit editor (mutually exclusive with RUNTIME)" ON)
option(GRAVIX_BUILD_SCRIPTING "Build C# scripting support" ON)

# Derive runtime build from editor option (mutually exclusive)
if(GRAVIX_BUILD_EDITOR)
    set(GRAVIX_BUILD_RUNTIME OFF)
else()
    set(GRAVIX_BUILD_RUNTIME ON)
endif()

# ============================================================================
# Platform Detection
# ============================================================================

if(WIN32)
    set(ENGINE_PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(ENGINE_PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(ENGINE_PLATFORM_MACOS TRUE)
endif()

# ============================================================================
# Global Compile Definitions
# ============================================================================

add_compile_definitions(
    $<$<CONFIG:Debug>:ENGINE_DEBUG>
    $<$<CONFIG:Release>:ENGINE_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:ENGINE_RELEASE>
    $<$<BOOL:${ENGINE_PLATFORM_WINDOWS}>:ENGINE_PLATFORM_WINDOWS>
    $<$<BOOL:${ENGINE_PLATFORM_LINUX}>:ENGINE_PLATFORM_LINUX>
    $<$<BOOL:${ENGINE_PLATFORM_MACOS}>:ENGINE_PLATFORM_MACOS>
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# ============================================================================
# Third-Party Dependencies
# ============================================================================

# Vulkan Bootstrap
set(VK_BOOTSTRAP_TEST OFF CACHE BOOL "" FORCE)
set(VK_BOOTSTRAP_WERROR OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParties/VkBootstrap)
set_target_properties(vk-bootstrap PROPERTIES FOLDER "Dependencies")

# EnkiTS Task System
set(ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParties/enkiTS)
set_target_properties(enkiTS PROPERTIES FOLDER "Dependencies")

# ============================================================================
# Editor-Only Dependencies
# ============================================================================

if(GRAVIX_BUILD_EDITOR)
    # ImGui
    add_subdirectory(ThirdParties/ImGui)
    set_target_properties(ImGui PROPERTIES FOLDER "Dependencies")

    # Slang Shader Compiler
    set(SLANG_ENABLE_GFX OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SLANG_ENABLE_SLANGRT ON CACHE BOOL "" FORCE)

    add_subdirectory(ThirdParties/Slang)
    set_target_properties(slang slang-glslang slang-rt core slangc
                           slang-common-objects compiler-core slangi
                           PROPERTIES FOLDER "Dependencies/Slang")

    # YAML-CPP
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(ThirdParties/yaml)
    set_target_properties(yaml-cpp PROPERTIES FOLDER "Dependencies")

    # ImGuizmo
    add_subdirectory(ThirdParties/ImGuizmo)
    set_target_properties(ImGuizmo PROPERTIES FOLDER "Dependencies")
endif()

# Box2D
add_subdirectory(ThirdParties/Box2D)
set_target_properties(box2d PROPERTIES FOLDER "Dependencies")

# Nvidia Streamline (RTX Support)
option(GRAVIX_ENABLE_STREAMLINE "Enable Nvidia Streamline for RTX features" ON)

if(GRAVIX_ENABLE_STREAMLINE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParties/Nvidia/Streamline/CMakeLists.txt")
        message(STATUS "Nvidia Streamline found - enabling RTX support")
        add_subdirectory(ThirdParties/Nvidia/Streamline)
        set(GRAVIX_HAS_STREAMLINE ON)
        add_compile_definitions(GRAVIX_HAS_STREAMLINE)
    else()
        message(WARNING "Nvidia Streamline directory not found at ThirdParties/Nvidia/Streamline")
        set(GRAVIX_HAS_STREAMLINE OFF)
    endif()
else()
    set(GRAVIX_HAS_STREAMLINE OFF)
endif()

# ============================================================================
# Engine Modules
# ============================================================================

if(GRAVIX_BUILD_SCRIPTING)
    add_subdirectory(Gravix-ScriptCore)
endif()

add_subdirectory(Gravix)

if(GRAVIX_BUILD_EDITOR)
    add_subdirectory(Orbit)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Orbit)
else()
    # Build runtime when editor is not being built
    add_subdirectory(Gravix-Runtime)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Gravix-Runtime)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "========================================")
message(STATUS "Gravix Engine Configuration")
message(STATUS "========================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Platform:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:   ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  Vulkan:         ${GRAVIX_USE_VULKAN}")
message(STATUS "  Editor:         ${GRAVIX_BUILD_EDITOR}")
message(STATUS "  Runtime:        ${GRAVIX_BUILD_RUNTIME}")
message(STATUS "  Scripting:      ${GRAVIX_BUILD_SCRIPTING}")
message(STATUS "  Streamline:     ${GRAVIX_HAS_STREAMLINE}")
message(STATUS "========================================")
