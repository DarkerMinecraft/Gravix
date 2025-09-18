cmake_minimum_required(VERSION 3.20)
project(Orbit)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)
# Create the Orbit executable TARGET FIRST
add_executable(Orbit ${SRC_FILES})

# NOW link libraries to the target (after it's been created)
target_link_libraries(Orbit
    PRIVATE
        Gravix    # Your main engine library
)

if(MSVC)
    target_compile_options(Orbit PRIVATE /utf-8)
endif()

# Include directories for the editor
target_include_directories(Orbit
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_include_directories(Orbit
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Gravix/Source/Core
        ${CMAKE_SOURCE_DIR}/Gravix/Source/Renderer/Generic
)

# Copy assets if needed
if(EXISTS ${CMAKE_SOURCE_DIR}/Assets)
    add_custom_command(TARGET Orbit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets $<TARGET_FILE_DIR:Orbit>/Assets
        COMMENT "Copying assets for Orbit Editor"
    )
endif()

# Copy editor-specific assets
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Assets)
    add_custom_command(TARGET Orbit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets $<TARGET_FILE_DIR:Orbit>/EditorAssets
        COMMENT "Copying editor assets for Orbit Editor"
    )
endif()

# Set as startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Orbit)

# Platform-specific settings
if(WIN32)
    # Set console subsystem for debugging, windows subsystem for release
    set_target_properties(Orbit PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
    
    # Add Windows-specific resources if available
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Orbit.rc)
        target_sources(Orbit PRIVATE Resources/Orbit.rc)
    endif()
elseif(APPLE)
    # macOS bundle settings
    set_target_properties(Orbit PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Orbit Editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.orbit"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# Debug information
message(STATUS "Orbit Editor configured successfully")
message(STATUS "  Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Engine dependency: Gravix::Engine")