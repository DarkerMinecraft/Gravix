# ============================================================================
# .NET Host Configuration
# ============================================================================

if(NOT GRAVIX_BUILD_SCRIPTING)
    return()
endif()

# Platform-specific .NET paths
if(WIN32)
    set(DOTNET_ROOT "C:/Program Files/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.win-x64/9.0.10/runtimes/win-x64/native")
elseif(UNIX AND NOT APPLE)
    set(DOTNET_ROOT "/usr/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.linux-x64/9.0.0/runtimes/linux-x64/native")
elseif(APPLE)
    set(DOTNET_ROOT "/usr/local/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.osx-x64/9.0.0/runtimes/osx-x64/native")
endif()

# Find nethost library
find_library(NETHOST_LIB
    NAMES nethost
    PATHS ${NETHOST_PATH}
    REQUIRED
)

if(NETHOST_LIB)
    message(STATUS ".NET Host found: ${NETHOST_LIB}")
    set(NETHOST_INCLUDE_DIR ${NETHOST_PATH})
else()
    message(FATAL_ERROR ".NET SDK not found! Please install .NET 9.0 SDK.")
endif()
