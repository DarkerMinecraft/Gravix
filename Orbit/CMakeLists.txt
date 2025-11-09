cmake_minimum_required(VERSION 3.20)
project(Orbit)

# ============================================================================
# Source Files
# ============================================================================

set(ORBIT_SOURCES
    # Application
    Source/App.cpp
    Source/AppLayer.cpp
    Source/AppLayer.h

    # Panels
    Source/Panels/ContentBrowserPanel.cpp
    Source/Panels/ContentBrowserPanel.h
    Source/Panels/InspectorPanel.cpp
    Source/Panels/InspectorPanel.h
    Source/Panels/ProjectSettingsPanel.cpp
    Source/Panels/ProjectSettingsPanel.h
    Source/Panels/SceneHierarchyPanel.cpp
    Source/Panels/SceneHierarchyPanel.h
    Source/Panels/ViewportPanel.cpp
    Source/Panels/ViewportPanel.h
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(Orbit ${ORBIT_SOURCES})

# Ensure Visual Studio tracks changes properly
set_target_properties(Orbit PROPERTIES
    FOLDER "Editor"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# UTF-8 support for MSVC
if(MSVC)
    target_compile_options(Orbit PRIVATE /utf-8)
endif()

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(Orbit
    PRIVATE
        Source
        ${CMAKE_SOURCE_DIR}/Gravix/Source/Core
        ${CMAKE_SOURCE_DIR}/ThirdParties/spdlog/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGui/
        ${CMAKE_SOURCE_DIR}/ThirdParties/glm
        ${CMAKE_SOURCE_DIR}/ThirdParties/yaml/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGuizmo
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(Orbit
    PRIVATE
        Gravix
        ImGui
        ImGuizmo
        yaml-cpp
)

# ============================================================================
# Asset Copying
# ============================================================================

# Root assets
file(GLOB_RECURSE ROOT_ASSET_FILES "${CMAKE_SOURCE_DIR}/Assets/*")
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/root_assets.stamp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:Orbit>/Assets"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/root_assets.stamp"
    DEPENDS ${ROOT_ASSET_FILES}
    COMMENT "Copying root assets..."
    VERBATIM
)

# Editor assets
file(GLOB_RECURSE EDITOR_ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*")
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/editor_assets.stamp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:Orbit>/EditorAssets"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/editor_assets.stamp"
    DEPENDS ${EDITOR_ASSET_FILES}
    COMMENT "Copying editor assets..."
    VERBATIM
)

add_custom_target(CopyRootAssets DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/root_assets.stamp")
add_custom_target(CopyEditorAssets DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/editor_assets.stamp")
set_target_properties(CopyRootAssets CopyEditorAssets PROPERTIES FOLDER "Editor/Assets")
add_dependencies(Orbit CopyRootAssets CopyEditorAssets)

# Slang DLL
add_custom_command(TARGET Orbit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:slang>"
        "$<TARGET_FILE_DIR:Orbit>"
    COMMENT "Copying Slang DLL..."
)

# ============================================================================
# Platform-Specific Settings
# ============================================================================

if(WIN32)
    # Console subsystem for debug, windows subsystem for release
    set_target_properties(Orbit PROPERTIES
        WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>
    )

    # Windows resources
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Orbit.rc)
        target_sources(Orbit PRIVATE Resources/Orbit.rc)
    endif()

elseif(APPLE)
    # macOS bundle
    set_target_properties(Orbit PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Orbit Editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gravix.orbit"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# ============================================================================
# Source Organization (IDE)
# ============================================================================

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" FILES ${ORBIT_SOURCES})

# Visual Studio startup project
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Orbit)
