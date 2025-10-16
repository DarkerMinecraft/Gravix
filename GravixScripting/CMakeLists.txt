cmake_minimum_required(VERSION 3.20)
project(GravixScripting)

add_library(GravixScripting STATIC)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)
target_sources(GravixScripting PRIVATE ${SRC_FILES})

if(MSVC)
    target_compile_options(GravixScripting PRIVATE /utf-8)
    
    # Define target architecture for Windows headers
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(GravixScripting PRIVATE _WIN64 _AMD64_ _M_AMD64)
    else()
        target_compile_definitions(GravixScripting PRIVATE _WIN32 _M_IX86)
    endif()
endif()

# --- Platform-specific .NET paths ---
if(WIN32)
    # Windows: Use installed .NET SDK
    set(DOTNET_ROOT "C:/Program Files/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.win-x64/9.0.10/runtimes/win-x64/native")
    
    target_include_directories(GravixScripting PRIVATE ${NETHOST_PATH})
    
    # Link nethost.lib
    find_library(NETHOST_LIB
        NAMES nethost
        PATHS ${NETHOST_PATH}
        REQUIRED
    )
    target_link_libraries(GravixScripting PRIVATE ${NETHOST_LIB})
    
elseif(UNIX AND NOT APPLE)
    # Linux: Use installed .NET SDK
    set(DOTNET_ROOT "/usr/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.linux-x64/9.0.0/runtimes/linux-x64/native")
    
    target_include_directories(GravixScripting PRIVATE ${NETHOST_PATH})
    
    find_library(NETHOST_LIB
        NAMES nethost
        PATHS ${NETHOST_PATH}
        REQUIRED
    )
    target_link_libraries(GravixScripting PRIVATE ${NETHOST_LIB} dl pthread)
    
elseif(APPLE)
    # macOS: Use installed .NET SDK
    set(DOTNET_ROOT "/usr/local/share/dotnet")
    set(NETHOST_PATH "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.osx-x64/9.0.0/runtimes/osx-x64/native")
    
    target_include_directories(GravixScripting PRIVATE ${NETHOST_PATH})
    
    find_library(NETHOST_LIB
        NAMES nethost
        PATHS ${NETHOST_PATH}
        REQUIRED
    )
    target_link_libraries(GravixScripting PRIVATE ${NETHOST_LIB})
endif()

# Public include directories
target_include_directories(GravixScripting 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/
)

# Link with Gravix core
target_link_libraries(GravixScripting PUBLIC Gravix)

# --- Optional: Add custom target to build C# scripts ---
find_program(DOTNET_EXECUTABLE dotnet)

if(DOTNET_EXECUTABLE)
    add_custom_target(BuildScripts ALL
        COMMAND ${DOTNET_EXECUTABLE} build "${CMAKE_SOURCE_DIR}/Scripts/GravixScripts.csproj" 
                -c Release -o "${CMAKE_BINARY_DIR}/Scripts"
        COMMENT "Building C# Scripts"
        VERBATIM
    )
    
    add_dependencies(GravixScripting BuildScripts)
endif()