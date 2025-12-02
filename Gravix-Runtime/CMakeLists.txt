cmake_minimum_required(VERSION 3.20)
project(Gravix-Runtime)

# ============================================================================
# Source Files
# ============================================================================

set(RUNTIME_SOURCES
    # Application
    Source/App.cpp
    Source/AppLayer.cpp
    Source/AppLayer.h
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(Gravix-Runtime ${RUNTIME_SOURCES})

# UTF-8 support for MSVC
if(MSVC)
    target_compile_options(Gravix-Runtime PRIVATE /utf-8)
endif()

# Define runtime build flag
target_compile_definitions(Gravix-Runtime PRIVATE GRAVIX_RUNTIME_BUILD)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(Gravix-Runtime
    PRIVATE
        Source
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(Gravix-Runtime
    PRIVATE
        Gravix
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
        "$<TARGET_FILE_DIR:Gravix-Runtime>/Assets"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/root_assets.stamp"
    DEPENDS ${ROOT_ASSET_FILES}
    COMMENT "Copying assets to Gravix-Runtime..."
    VERBATIM
)

add_custom_target(CopyRuntimeAssets DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/root_assets.stamp")
set_target_properties(CopyRuntimeAssets PROPERTIES FOLDER "Runtime/Assets")

add_dependencies(Gravix-Runtime CopyRuntimeAssets)

# Always build ScriptCore before Gravix-Runtime (if scripting is enabled)
if(GRAVIX_BUILD_SCRIPTING)
    add_dependencies(Gravix-Runtime Gravix-ScriptCore)
endif()

# NOTE: Slang, ImGui, YAML-CPP are not needed in runtime builds
# Runtime uses pre-compiled shaders and binary asset formats

# Gravix-ScriptCore DLL and dependencies (if scripting is enabled)
if(GRAVIX_BUILD_SCRIPTING)
    # Create a custom target that always copies the scripting DLL
    set(SCRIPTCORE_DLL "${CMAKE_BINARY_DIR}/Gravix-ScriptCore/GravixScripting.dll")

    add_custom_target(CopyRuntimeScriptCoreDLL ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPTCORE_DLL}"
            "$<TARGET_FILE_DIR:Gravix-Runtime>/GravixScripting.dll"
        DEPENDS "${SCRIPTCORE_DLL}" Gravix-ScriptCore
        COMMENT "Copying GravixScripting.dll to Gravix-Runtime output directory..."
        VERBATIM
    )

    add_dependencies(Gravix-Runtime CopyRuntimeScriptCoreDLL)
    set_target_properties(CopyRuntimeScriptCoreDLL PROPERTIES FOLDER "Runtime/Assets")

    # Copy Mono runtime folder only once (use stamp file)
    set(MONO_RUNTIME_SOURCE "C:/Program Files/Mono/lib/mono/4.5")
    set(MONO_RUNTIME_DEST "$<TARGET_FILE_DIR:Gravix-Runtime>/lib/mono/4.5")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MONO_RUNTIME_DEST}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MONO_RUNTIME_SOURCE}"
            "${MONO_RUNTIME_DEST}"
        COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp"
        COMMENT "Copying Mono 4.5 runtime to Gravix-Runtime (one-time)..."
        VERBATIM
    )

    add_custom_target(CopyRuntimeMonoRuntime DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mono_runtime.stamp")
    add_dependencies(Gravix-Runtime CopyRuntimeMonoRuntime)
endif()

# ============================================================================
# Platform-Specific Settings
# ============================================================================

if(WIN32)
    # Console subsystem for debug, windows subsystem for release
    set_target_properties(Gravix-Runtime PROPERTIES
        WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>
    )

    # Windows resources (if exists)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Gravix-Runtime.rc)
        target_sources(Gravix-Runtime PRIVATE Resources/Gravix-Runtime.rc)
    endif()

elseif(APPLE)
    # macOS bundle
    set_target_properties(Gravix-Runtime PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Gravix Runtime"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gravix.runtime"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

# ============================================================================
# Source Organization (IDE)
# ============================================================================

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" FILES ${RUNTIME_SOURCES})
