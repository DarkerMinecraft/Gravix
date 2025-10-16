cmake_minimum_required(VERSION 3.20)
project(GravixEngine VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    set(ENGINE_PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(ENGINE_PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(ENGINE_PLATFORM_MACOS TRUE)
endif()

option(ENGINE_USE_VULKAN ON)

# Global compile definitions
add_compile_definitions(
    $<$<CONFIG:Debug>:ENGINE_DEBUG>
    $<$<CONFIG:Release>:ENGINE_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:ENGINE_RELEASE>
    $<$<BOOL:${ENGINE_PLATFORM_WINDOWS}>:ENGINE_PLATFORM_WINDOWS>
    $<$<BOOL:${ENGINE_PLATFORM_LINUX}>:ENGINE_PLATFORM_LINUX>
    $<$<BOOL:${ENGINE_PLATFORM_MACOS}>:ENGINE_PLATFORM_MACOS>
)

add_subdirectory(ThirdParties/VkBootstrap)
set_target_properties(vk-bootstrap PROPERTIES FOLDER "Dependencies")

set(ENKITS_BUILD_EXAMPLES OFF)
add_subdirectory(ThirdParties/enkiTS)
set_target_properties(enkiTS PROPERTIES FOLDER "Dependencies")

add_subdirectory(ThirdParties/ImGui)
set_target_properties(ImGui PROPERTIES FOLDER "Dependencies")

set(SLANG_ENABLE_GFX OFF CACHE BOOL "" FORCE)             # Disable gfx integration
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "" FORCE)          # Disable language server
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)           # Disable tests
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)        # Disable samples/examples
set(SLANG_ENABLE_SLANGRT ON CACHE BOOL "" FORCE)          # Enable runtime (reflection, compiler core)

add_subdirectory(ThirdParties/Slang)
set_target_properties(slang PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(slang-glslang PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(slang-rt PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(core PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(slangc PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(slang-common-objects PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(compiler-core PROPERTIES FOLDER "Slang/Dependencies")
set_target_properties(slangi PROPERTIES FOLDER "Slang/Dependencies")

# Add engine modules
add_subdirectory(GravixScripting)
add_subdirectory(Gravix)
add_subdirectory(Orbit)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Orbit)