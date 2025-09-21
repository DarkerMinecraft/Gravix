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
    $<$<BOOL:${ENGINE_PLATFORM_WINDOWS}>:ENGINE_PLATFORM_WINDOWS>
    $<$<BOOL:${ENGINE_PLATFORM_LINUX}>:ENGINE_PLATFORM_LINUX>
    $<$<BOOL:${ENGINE_PLATFORM_MACOS}>:ENGINE_PLATFORM_MACOS>
)

# Add engine modules
add_subdirectory(Gravix)
add_subdirectory(Orbit)

add_subdirectory(ThirdParties/VkBootstrap)
set_target_properties(vk-bootstrap PROPERTIES FOLDER "Dependencies")

set(ENKITS_BUILD_EXAMPLES OFF)
add_subdirectory(ThirdParties/enkiTS)
set_target_properties(enkiTS PROPERTIES FOLDER "Dependencies")

add_subdirectory(ThirdParties/ImGui)
set_target_properties(ImGui PROPERTIES FOLDER "Dependencies")
