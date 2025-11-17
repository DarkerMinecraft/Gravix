# ============================================================================
# .NET Host Configuration
# ============================================================================

if(NOT GRAVIX_BUILD_SCRIPTING)
    return()
endif()

# Find dotnet executable
find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

if(DOTNET_EXECUTABLE)
    # Get the .NET runtime version
    execute_process(
        COMMAND ${DOTNET_EXECUTABLE} --list-runtimes
        OUTPUT_VARIABLE DOTNET_RUNTIMES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Extract Microsoft.NETCore.App versions and prefer 9.x
    string(REGEX MATCHALL "Microsoft\\.NETCore\\.App ([0-9]+\\.[0-9]+\\.[0-9]+)" DOTNET_RUNTIME_MATCHES "${DOTNET_RUNTIMES}")

    # Try to find 9.x version first
    set(DOTNET_RUNTIME_VERSION "")
    foreach(MATCH_ITEM ${DOTNET_RUNTIME_MATCHES})
        string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" VERSION_ONLY "${MATCH_ITEM}")
        if(VERSION_ONLY MATCHES "^9\\.")
            set(DOTNET_RUNTIME_VERSION ${VERSION_ONLY})
            break()
        endif()
    endforeach()

    # If no 9.x found, use the last found version
    if(NOT DOTNET_RUNTIME_VERSION)
        if(DOTNET_RUNTIME_MATCHES)
            list(GET DOTNET_RUNTIME_MATCHES -1 LAST_MATCH)
            string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" DOTNET_RUNTIME_VERSION "${LAST_MATCH}")
        else()
            message(FATAL_ERROR "Could not detect .NET runtime version")
        endif()
    endif()

    message(STATUS ".NET Runtime version: ${DOTNET_RUNTIME_VERSION}")
endif()

# Platform-specific .NET paths
if(WIN32)
    set(DOTNET_ROOT "C:/Program Files/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.win-x64/${DOTNET_RUNTIME_VERSION}/runtimes/win-x64/native")
elseif(UNIX AND NOT APPLE)
    set(DOTNET_ROOT "/usr/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.linux-x64/${DOTNET_RUNTIME_VERSION}/runtimes/linux-x64/native")
elseif(APPLE)
    set(DOTNET_ROOT "/usr/local/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.osx-x64/${DOTNET_RUNTIME_VERSION}/runtimes/osx-x64/native")
endif()

message(STATUS "NetHost path: ${NETHOST_PATH}")

# Find nethost library
find_library(NETHOST_LIB
    NAMES nethost
    PATHS ${NETHOST_PATH}
    REQUIRED
)

if(NETHOST_LIB)
    message(STATUS ".NET Host found: ${NETHOST_LIB}")
    set(NETHOST_INCLUDE_DIR ${NETHOST_PATH})

    # On Windows, also locate the DLL (the .lib is just the import library)
    if(WIN32)
        get_filename_component(NETHOST_DIR ${NETHOST_LIB} DIRECTORY)
        set(NETHOST_DLL "${NETHOST_DIR}/nethost.dll")
        if(EXISTS ${NETHOST_DLL})
            message(STATUS ".NET Host DLL found: ${NETHOST_DLL}")
        else()
            message(WARNING ".NET Host DLL not found at: ${NETHOST_DLL}")
        endif()
    endif()
else()
    message(FATAL_ERROR ".NET SDK not found! Please install .NET 9.0 SDK.")
endif()
