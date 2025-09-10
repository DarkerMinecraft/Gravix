# Define source files for the Orbit editor
set(ORBIT_SOURCES
    src/main.cpp
    
)

# Create the Orbit executable TARGET FIRST
add_executable(Orbit ${ORBIT_SOURCES})

# Set target properties
set_target_properties(Orbit PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# NOW link libraries to the target (after it's been created)
target_link_libraries(Orbit
    PRIVATE
        Gravix::Engine    # Your main engine library
)

# Include directories for the editor
target_include_directories(Orbit
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add compile definitions for the editor
target_compile_definitions(Orbit
    PRIVATE
        ORBIT_EDITOR=1
        $<$<CONFIG:Debug>:ORBIT_DEBUG>
        $<$<CONFIG:Release>:ORBIT_RELEASE>
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

# Optional: Set up IDE folders
set_target_properties(Orbit PROPERTIES FOLDER "Applications")

# Debug information
message(STATUS "Orbit Editor configured successfully")
message(STATUS "  Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Engine dependency: Engine::GameEngine")