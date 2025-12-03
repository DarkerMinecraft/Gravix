cmake_minimum_required(VERSION 3.20)
project(Gravix)

# ============================================================================
# Configuration Modules
# ============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(VulkanConfig)

# ============================================================================
# Core Source Files
# ============================================================================

set(GRAVIX_CORE_SOURCES
    # Core
    Source/Core/Application.cpp
    Source/Core/Console.cpp
    Source/Core/Log.cpp
    Source/Core/Scheduler.cpp
    Source/Core/UUID.cpp

    # Asset System (Core)
    Source/Asset/Asset.cpp
    Source/Asset/RuntimeAssetManager.cpp

    # Platform
    Source/Platform/Windows/WindowsInput.cpp
    Source/Platform/Windows/WindowsPlatformUtils.cpp
    Source/Platform/Windows/WindowsWindow.cpp

    # Project
    Source/Project/Project.cpp

    # Renderer (Generic)
    Source/Renderer/Generic/Command.cpp
    Source/Renderer/Generic/Renderer2D.cpp
    Source/Renderer/Generic/Types/Framebuffer.cpp
    Source/Renderer/Generic/Types/Material.cpp
    Source/Renderer/Generic/Types/Mesh.cpp
    Source/Renderer/Generic/Types/Pipeline.cpp
    Source/Renderer/Generic/Types/Shader.cpp
    Source/Renderer/Generic/Types/Texture.cpp

    # Scene
    Source/Scene/ComponentRegistry.cpp
    Source/Scene/Scene.cpp
    Source/Scene/SceneCamera.cpp

    # Scene - Component Renderers
    Source/Scene/ComponentRenderers/TagComponentRenderer.cpp
    Source/Scene/ComponentRenderers/TransformComponentRenderer.cpp
    Source/Scene/ComponentRenderers/CameraComponentRenderer.cpp
    Source/Scene/ComponentRenderers/SpriteRendererComponentRenderer.cpp
    Source/Scene/ComponentRenderers/ScriptComponentRenderer.cpp
    Source/Scene/ComponentRenderers/Rigidbody2DComponentRenderer.cpp
    Source/Scene/ComponentRenderers/BoxCollider2DComponentRenderer.cpp
    Source/Scene/ComponentRenderers/CircleRendererComponentRenderer.cpp
    Source/Scene/ComponentRenderers/CircleCollider2DComponentRenderer.cpp
    Source/Scene/ComponentRenderers/ComponentOrderComponentRenderer.cpp

    # Scripting - Core
    Source/Scripting/Core/ScriptEngine.cpp
    Source/Scripting/Core/ScriptTypes.cpp

    # Scripting - Editor
    Source/Scripting/Editor/EditorScriptEngine.cpp

    # Scripting - Runtime
    Source/Scripting/Runtime/RuntimeScriptEngine.cpp

    # Scripting - Fields
    Source/Scripting/Fields/ScriptFieldHandler.cpp
    Source/Scripting/Fields/ScriptFieldRegistry.cpp

    # Scripting - Interop
    Source/Scripting/Interop/ScriptGlue.cpp
    Source/Scripting/Interop/ScriptUtils.cpp

    # Serialization (Core - used in both editor and runtime)
    Source/Serialization/Scene/SceneSerializer.cpp

    # Math
    Source/Maths/Maths.cpp

    # Physics
    Source/Physics/PhysicsWorld.cpp
)

# ============================================================================
# Editor-Specific Source Files (Conditional)
# ============================================================================

if(GRAVIX_BUILD_EDITOR)
    set(GRAVIX_EDITOR_SOURCES
        # Debug (Editor)
        Source/Debug/ProfilerViewer.cpp

        # Editor UI
        Source/Renderer/ImGuiBuild.cpp
        Source/Renderer/ImGuiRender.cpp
        Source/Scene/ImGuiHelpers.cpp

        # Editor Camera
        Source/Scene/EditorCamera.cpp

        # Editor Asset System
        Source/Asset/EditorAssetManager.cpp
        Source/Asset/AssetImporter.cpp
        Source/Asset/Importers/SceneImporter.cpp
        Source/Asset/Importers/TextureImporter.cpp
        Source/Asset/Importers/ShaderImporter.cpp
        Source/Asset/Importers/PipelineImporter.cpp
        Source/Asset/Importers/MaterialImporter.cpp

        # Editor Serialization (YAML-based)
        Source/Serialization/Project/ProjectSerializer.cpp

        # Shader Compilation (Editor only - runtime uses pre-compiled)
        Source/Utils/ShaderCompiler.cpp
        Source/Utils/ShaderCompilerSystem.cpp
        Source/Utils/ShaderReflector.cpp
        Source/Utils/SlangTypeUtils.cpp
    )
else()
    set(GRAVIX_EDITOR_SOURCES "")
endif()

# ============================================================================
# Vulkan-Specific Source Files (Conditional)
# ============================================================================

if(GRAVIX_USE_VULKAN)
    set(GRAVIX_VULKAN_SOURCES
        # Vulkan Implementation
        Source/Renderer/Vulkan/VulkanCommandImpl.cpp
        Source/Renderer/Vulkan/VulkanDevice.cpp
        Source/Renderer/Vulkan/VulkanSwapchain.cpp
        Source/Renderer/Vulkan/VulkanRenderCaps.cpp

        # Vulkan Types
        Source/Renderer/Vulkan/Types/VulkanFramebuffer.cpp
        Source/Renderer/Vulkan/Types/VulkanMaterial.cpp
        Source/Renderer/Vulkan/Types/VulkanMesh.cpp
        Source/Renderer/Vulkan/Types/VulkanShader.cpp
        Source/Renderer/Vulkan/Types/VulkanTexture.cpp

        # Vulkan Utils
        Source/Renderer/Vulkan/Utils/DescriptorWriter.cpp
        Source/Renderer/Vulkan/Utils/VulkanInitializers.cpp
        Source/Renderer/Vulkan/Utils/VulkanUtils.cpp
        Source/Renderer/Vulkan/Utils/VulkanDeviceInit.cpp
        Source/Renderer/Vulkan/Utils/VulkanDescriptorSetup.cpp
        Source/Renderer/Vulkan/Utils/VulkanCommandSetup.cpp
    )
else()
    set(GRAVIX_VULKAN_SOURCES "")
endif()

# ============================================================================
# Library Target
# ============================================================================

add_library(Gravix STATIC
    ${GRAVIX_CORE_SOURCES}
    ${GRAVIX_EDITOR_SOURCES}
    ${GRAVIX_VULKAN_SOURCES}
)

# Precompiled header
target_precompile_headers(Gravix PRIVATE Source/pch.h)

# UTF-8 support for MSVC
if(MSVC)
    target_compile_options(Gravix PRIVATE /utf-8)
endif()

# Editor build definition
if(GRAVIX_BUILD_EDITOR)
    target_compile_definitions(Gravix PRIVATE GRAVIX_EDITOR_BUILD)
else()
    target_compile_definitions(Gravix PRIVATE GRAVIX_RUNTIME_BUILD)
endif()

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(Gravix
    PUBLIC
        Source/
        ${CMAKE_SOURCE_DIR}/ThirdParties/spdlog/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/glm
        ${CMAKE_SOURCE_DIR}/ThirdParties/entt/single_include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParties/enkiTS/src
        ${CMAKE_SOURCE_DIR}/ThirdParties/Box2D/include
)

# Vulkan-specific includes
if(GRAVIX_USE_VULKAN)
    target_include_directories(Gravix
        PRIVATE
            ${Vulkan_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/ThirdParties/VkBootstrap/src
            ${CMAKE_SOURCE_DIR}/ThirdParties/VulkanMemoryAllocator/include
    )
endif()

# Editor-only includes (ImGui, ImGuizmo, YAML, Slang, stb)
if(GRAVIX_BUILD_EDITOR)
    target_include_directories(Gravix
        PUBLIC
            ${CMAKE_SOURCE_DIR}/ThirdParties/ImGui
            ${CMAKE_SOURCE_DIR}/ThirdParties/ImGuizmo
            ${CMAKE_SOURCE_DIR}/ThirdParties/Slang/include
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ThirdParties/yaml/include
            ${CMAKE_SOURCE_DIR}/ThirdParties/stb/
    )
endif()

# Streamline includes
if(GRAVIX_HAS_STREAMLINE)
    target_include_directories(Gravix
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ThirdParties/Nvidia/Streamline/include
    )
endif()

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(Gravix
    PRIVATE
        enkiTS
        box2d
)

# Editor-only dependencies
if(GRAVIX_BUILD_EDITOR)
    target_link_libraries(Gravix
        PRIVATE
            ImGui
            ImGuizmo
            yaml-cpp
            slang
    )
endif()

# Vulkan-specific libraries
if(GRAVIX_USE_VULKAN)
    target_link_libraries(Gravix
        PRIVATE
            ${Vulkan_LIBRARIES}
            vk-bootstrap
    )
endif()

if(GRAVIX_BUILD_SCRIPTING)
    target_link_libraries(Gravix
        PRIVATE
            debug "${CMAKE_SOURCE_DIR}/ThirdParties/mono/lib/Debug/libmono-static-sgen.lib"
            optimized "${CMAKE_SOURCE_DIR}/ThirdParties/mono/lib/Release/libmono-static-sgen.lib"
    )
    target_include_directories(Gravix
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ThirdParties/mono/include
    )

    # Define MONO_STATIC_BUILD for static linking
    target_compile_definitions(Gravix PRIVATE MONO_STATIC_BUILD)

    # Ensure Gravix-ScriptCore DLL is always built before Gravix
    add_dependencies(Gravix Gravix-ScriptCore)

    # Windows dependencies required for static Mono linking
    if(WIN32)
        target_link_libraries(Gravix
            PRIVATE
                Ws2_32.lib      # WinSock
                Winmm.lib       # Windows Multimedia
                Version.lib     # Version API
                Bcrypt.lib      # Windows Cryptography API
        )
    endif()
endif()

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(Gravix PRIVATE dl pthread)
endif()

# ============================================================================
# Source Organization (IDE)
# ============================================================================

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" FILES ${GRAVIX_CORE_SOURCES} ${GRAVIX_EDITOR_SOURCES} ${GRAVIX_VULKAN_SOURCES})
