# CMake Configuration Modules

This directory contains modular CMake configuration files for the Gravix Engine.

## Modules

### VulkanConfig.cmake
Configures Vulkan SDK integration.

- Searches for Vulkan SDK
- Sets up include directories and libraries
- Adds `ENGINE_USE_VULKAN` compile definition
- Controlled by `GRAVIX_USE_VULKAN` option

### NetHostConfig.cmake
Configures .NET Core hosting for C# scripting support.

- Locates .NET 9.0 SDK installation
- Finds nethost library for .NET hosting
- Platform-specific paths (Windows/Linux/macOS)
- Controlled by `GRAVIX_BUILD_SCRIPTING` option

## Usage

These modules are automatically included by `Gravix/CMakeLists.txt`:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(VulkanConfig)
include(NetHostConfig)
```

## Adding New Modules

To add a new configuration module:

1. Create `YourFeature.cmake` in this directory
2. Add an option in root `CMakeLists.txt`:
   ```cmake
   option(GRAVIX_USE_YOUR_FEATURE "Description" ON)
   ```
3. Include the module in `Gravix/CMakeLists.txt`:
   ```cmake
   include(YourFeature)
   ```
4. Use early return if disabled:
   ```cmake
   if(NOT GRAVIX_USE_YOUR_FEATURE)
       return()
   endif()
   ```
