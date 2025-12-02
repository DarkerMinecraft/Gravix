# RuntimeConfig.cmake
# Configuration for runtime/shipping builds without editor features

message(STATUS "Configuring Gravix for Runtime Build")

# Force disable editor features
set(GRAVIX_BUILD_EDITOR OFF CACHE BOOL "Disable editor for runtime build" FORCE)

# Add runtime-specific compile definitions
add_compile_definitions(
    GRAVIX_RUNTIME_BUILD
    GRAVIX_DISABLE_SHADER_COMPILATION
    GRAVIX_DISABLE_EDITOR_ASSETS
)

# Runtime build should use Release optimizations by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Runtime Configuration Applied:")
message(STATUS "  - Editor: DISABLED")
message(STATUS "  - Shader Compilation: DISABLED")
message(STATUS "  - Editor Assets: DISABLED")
message(STATUS "  - Asset Manager: RuntimeAssetManager only")
