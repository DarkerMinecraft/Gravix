cmake_minimum_required(VERSION 3.20)
project(Gravix-ScriptCore CSharp)

# Collect all C# source files
file(GLOB_RECURSE CS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cs")

set(SCRIPTCORE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Gravix-ScriptCore")

# Create C# library target
add_library(Gravix-ScriptCore SHARED ${CS_FILES})

# Set C# project properties
set_target_properties(Gravix-ScriptCore PROPERTIES
    VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.5"
    VS_DOTNET_REFERENCES "System"
    DOTNET_TARGET_FRAMEWORK_VERSION "v4.5"
    OUTPUT_NAME "GravixScripting"
    RUNTIME_OUTPUT_DIRECTORY "${SCRIPTCORE_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${SCRIPTCORE_OUTPUT_DIR}"
    FOLDER "Scripting"
    VS_GLOBAL_ROOTNAMESPACE "GravixEngine"
)

# ----------------------------------------
# Visual Studio source grouping
# ----------------------------------------

foreach(SOURCE_FILE ${CS_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${SOURCE_FILE}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    string(REPLACE "/" "\\" GROUP_PATH "${REL_DIR}")
    source_group("${GROUP_PATH}" FILES "${SOURCE_FILE}")
endforeach()
