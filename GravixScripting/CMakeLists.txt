cmake_minimum_required(VERSION 3.20)
project(GravixScripting)

add_library(GravixScripting STATIC)
add_dependencies(GravixScripting Gravix-ScriptCore)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)
target_sources(GravixScripting PRIVATE ${SRC_FILES})

target_compile_definitions(GravixScripting PRIVATE GRAVIXSCRIPTING_EXPORTS)

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
    
        find_library(NETHOST_LIB
        NAMES nethost
        PATHS ${NETHOST_PATH}
        REQUIRED
    )
    target_link_libraries(GravixScripting PUBLIC ${NETHOST_LIB})
    
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
    target_link_libraries(GravixScripting PUBLIC ${NETHOST_LIB} dl pthread)
    
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
    target_link_libraries(GravixScripting PUBLIC ${NETHOST_LIB})
endif()

add_custom_command(TARGET GravixScripting POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NETHOST_PATH}/nethost.dll"
            $<TARGET_FILE_DIR:GravixScripting>
        COMMENT "Copying nethost.dll to GravixScripting output directory"
    )

# Build the managed .NET assembly (framework-dependent)
find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

set(MANAGED_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Managed")
set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/GravixScripting/Managed")

# Create a custom target that ALWAYS builds the managed assembly
add_custom_target(GravixScriptingManaged ALL
    COMMAND ${DOTNET_EXECUTABLE} publish 
        "${MANAGED_PROJECT_DIR}/GravixScripting.csproj"
        --configuration $<CONFIG>
        --output "${MANAGED_OUTPUT_DIR}"
        --no-self-contained
    WORKING_DIRECTORY ${MANAGED_PROJECT_DIR}
    COMMENT "Building framework-dependent managed GravixScripting assembly"
)

# Make GravixScripting depend on the managed build
add_dependencies(GravixScripting GravixScriptingManaged)

# Copy only the managed DLL and runtimeconfig.json
add_custom_command(TARGET GravixScripting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MANAGED_OUTPUT_DIR}/GravixScripting.dll"
        $<TARGET_FILE_DIR:GravixScripting>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MANAGED_OUTPUT_DIR}/GravixScripting.runtimeconfig.json"
        $<TARGET_FILE_DIR:GravixScripting>
    COMMENT "Copying managed assembly and runtime config"
)

# Public include directories
target_include_directories(GravixScripting 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/
)