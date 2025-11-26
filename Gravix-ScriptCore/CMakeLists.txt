cmake_minimum_required(VERSION 3.20)
project(Gravix-ScriptCore)

# Find Mono C# compiler
find_program(MONO_CSHARP_COMPILER
    NAMES csc.bat csc mcs
    PATH_SUFFIXES bin
    REQUIRED
)

set(SCRIPTCORE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Gravix-ScriptCore")
file(MAKE_DIRECTORY "${SCRIPTCORE_OUTPUT_DIR}")

file(GLOB_RECURSE CS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cs")

set(OUTPUT_DLL "${SCRIPTCORE_OUTPUT_DIR}/GravixScripting.dll")

# Only rebuild when C# source files change
add_custom_command(
    OUTPUT "${OUTPUT_DLL}"
    COMMAND "${MONO_CSHARP_COMPILER}"
        -target:library
        -out:"${OUTPUT_DLL}"
        ${CS_FILES}
    DEPENDS ${CS_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Gravix-ScriptCore C# project with Mono compiler"
)

add_custom_target(Gravix-ScriptCore ALL
    DEPENDS "${OUTPUT_DLL}"
    SOURCES ${CS_FILES}
)

# ----------------------------------------
# Visual Studio source grouping (unchanged)
# ----------------------------------------

foreach(SOURCE_FILE ${CS_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${SOURCE_FILE}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    string(REPLACE "/" "\\" GROUP_PATH "${REL_DIR}")
    source_group("${GROUP_PATH}" FILES "${SOURCE_FILE}")
endforeach()
