cmake_minimum_required(VERSION 3.20)
project(Gravix-ScriptCore)

# Find dotnet executable
find_program(DOTNET_EXECUTABLE dotnet REQUIRED)

# Set output directory
set(SCRIPTCORE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Gravix-ScriptCore")

# Gather all C# files
file(GLOB_RECURSE CS_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cs"
)

# Gather the .csproj file
set(CSPROJ_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Gravix-ScriptCore.csproj")

# Create a custom target for the C# project
add_custom_target(Gravix-ScriptCore ALL
    COMMAND ${DOTNET_EXECUTABLE} build 
        "${CSPROJ_FILE}"
        --configuration $<CONFIG>
        --output "${SCRIPTCORE_OUTPUT_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Gravix-ScriptCore C# project"
    SOURCES ${CSPROJ_FILE} ${CS_FILES}
)

# Organize files in Visual Studio to match filesystem structure
foreach(SOURCE_FILE ${CS_FILES})
    # Get the relative path from the project root
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${SOURCE_FILE}")
    
    # Get the directory part of the path
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    
    # Convert forward slashes to backslashes for Windows
    string(REPLACE "/" "\\" GROUP_PATH "${REL_DIR}")
    
    # Create the source group (folder in VS)
    source_group("${GROUP_PATH}" FILES "${SOURCE_FILE}")
endforeach()

# Also organize the .csproj file
source_group("" FILES ${CSPROJ_FILE})