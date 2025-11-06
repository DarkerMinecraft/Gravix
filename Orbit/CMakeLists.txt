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
        ImGui
        ImGuizmo
        yaml-cpp
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

        ${CMAKE_SOURCE_DIR}/ThirdParties/spdlog/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGui/
        ${CMAKE_SOURCE_DIR}/ThirdParties/glm
        ${CMAKE_SOURCE_DIR}/ThirdParties/yaml/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGuizmo
)

# Copy root assets every build
add_custom_target(CopyRootAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Orbit>/Assets
    COMMENT "Copying root assets for Orbit Editor"
)

# Copy editor-specific assets every build
add_custom_target(CopyEditorAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Orbit>/EditorAssets
    COMMENT "Copying editor assets for Orbit Editor"
)

# Make Orbit depend on the copy targets
add_dependencies(Orbit CopyRootAssets CopyEditorAssets)

# Set as startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Orbit)

# Platform-specific settings
if(WIN32)
    # Set console subsystem for debugging, windows subsystem for release
    set_target_properties(Orbit PROPERTIES
        WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>
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

add_custom_target(CopyScripting ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/GravixScripting/Managed/GravixScripting.dll"
        $<TARGET_FILE_DIR:Orbit>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/GravixScripting/Managed/GravixScripting.runtimeconfig.json"
        $<TARGET_FILE_DIR:Orbit>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/GravixScripting/nethost.dll"
        $<TARGET_FILE_DIR:Orbit>
    COMMENT "Copying GravixScripting config and managed DLLs"
)

# Make CopyScripting wait for GravixScripting to build first
add_dependencies(CopyScripting GravixScripting)
add_dependencies(Orbit CopyScripting)

add_custom_command(TARGET Orbit POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:slang>
    $<TARGET_FILE_DIR:Orbit>)

# Debug information
message(STATUS "Orbit Editor configured successfully")
message(STATUS "  Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Engine dependency: Gravix::Engine")