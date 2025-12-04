cmake_minimum_required(VERSION 3.20)
project(Orbit)

# ============================================================================
# Source Files
# ============================================================================

set(ORBIT_SOURCES
    # Application
    Source/App.cpp
    Source/AppLayer.cpp
    Source/SplashScreen.cpp

    # Managers
    Source/ProjectManager.cpp
    Source/SceneManager.cpp

    # Panels
    Source/Panels/ConsolePanel.cpp
    Source/Panels/ContentBrowserPanel.cpp
    Source/Panels/InspectorPanel.cpp
    Source/Panels/ProjectSettingsPanel.cpp
    Source/Panels/SceneHierarchyPanel.cpp
    Source/Panels/ViewportPanel.cpp

    # UI Components
    Source/UI/EditorMenuBar.cpp
    Source/UI/EditorToolbar.cpp

    # Utils
    Source/Utils/AssetFactory.cpp
    Source/Utils/ExternalEditorLauncher.cpp
    Source/Utils/KeyboardShortcutHandler.cpp
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(Orbit ${ORBIT_SOURCES})

# UTF-8 support for MSVC
if(MSVC)
    target_compile_options(Orbit PRIVATE /utf-8)
endif()

# Define editor build flag
target_compile_definitions(Orbit PRIVATE GRAVIX_EDITOR_BUILD)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(Orbit
    PRIVATE
        Source
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

# Always build ScriptCore before Orbit (if scripting is enabled)
if(GRAVIX_BUILD_SCRIPTING)
    add_dependencies(Orbit Gravix-ScriptCore)
endif()

# Gravix-ScriptCore DLL and dependencies (if scripting is enabled)
if(GRAVIX_BUILD_SCRIPTING)
    # Create a custom target that always copies the scripting DLL
    # This ensures C# changes are always reflected in the output directory
    set(SCRIPTCORE_DLL "${CMAKE_BINARY_DIR}/Gravix-ScriptCore/GravixScripting.dll")

    add_custom_target(CopyScriptCoreDLL ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPTCORE_DLL}"
            "$<TARGET_FILE_DIR:Orbit>/GravixScripting.dll"
        DEPENDS "${SCRIPTCORE_DLL}" Gravix-ScriptCore
        COMMENT "Copying GravixScripting.dll to Orbit output directory..."
        VERBATIM
    )

    add_dependencies(Orbit CopyScriptCoreDLL)
    set_target_properties(CopyScriptCoreDLL PROPERTIES FOLDER "Editor/Assets")

    # Copy Mono runtime folder only once (use stamp file)
    set(MONO_RUNTIME_SOURCE "C:/Program Files/Mono/lib/mono/4.5")
    set(MONO_RUNTIME_DEST "$<TARGET_FILE_DIR:Orbit>/lib/mono/4.5")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MONO_RUNTIME_DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MONO_RUNTIME_SOURCE}"
            "${MONO_RUNTIME_DEST}"
        COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp"
        COMMENT "Copying Mono 4.5 runtime (one-time)..."
        VERBATIM
    )

    add_custom_target(CopyMonoRuntime DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp")
    add_dependencies(Orbit CopyMonoRuntime)
endif()


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
