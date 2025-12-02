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
    Source/Core/Application.h
    Source/Core/Buffer.h
    Source/Core/Core.h
    Source/Core/Gravix.h
    Source/Core/Input.h
    Source/Core/Layer.h
    Source/Core/Log.cpp
    Source/Core/Log.h
    Source/Core/RefCounted.h
    Source/Core/Scheduler.cpp
    Source/Core/Scheduler.h
    Source/Core/UUID.cpp
    Source/Core/UUID.h
    Source/Core/Window.h

    # Asset System (Core)
    Source/Asset/Asset.cpp
    Source/Asset/Asset.h
    Source/Asset/AssetManager.h
    Source/Asset/AssetManagerBase.h
    Source/Asset/AssetMetadata.h
    Source/Asset/AsyncLoadRequest.h
    Source/Asset/RuntimeAssetManager.cpp
    Source/Asset/RuntimeAssetManager.h

    # Events
    Source/Events/Event.h
    Source/Events/KeyEvents.h
    Source/Events/MouseEvents.h
    Source/Events/WindowEvents.h

    # Platform
    Source/Platform/Windows/WindowsInput.cpp
    Source/Platform/Windows/WindowsPlatformUtils.cpp
    Source/Platform/Windows/WindowsWindow.cpp
    Source/Platform/Windows/WindowsWindow.h

    # Project
    Source/Project/Project.cpp
    Source/Project/Project.h

    # Renderer (Generic)
    Source/Renderer/CommandImpl.h
    Source/Renderer/Specification.h
    Source/Renderer/Generic/Camera.h
    Source/Renderer/Generic/Command.cpp
    Source/Renderer/Generic/Command.h
    Source/Renderer/Generic/Device.h
    Source/Renderer/Generic/Renderer2D.cpp
    Source/Renderer/Generic/Renderer2D.h
    Source/Renderer/Generic/Types/Framebuffer.cpp
    Source/Renderer/Generic/Types/Framebuffer.h
    Source/Renderer/Generic/Types/Material.cpp
    Source/Renderer/Generic/Types/Material.h
    Source/Renderer/Generic/Types/Mesh.cpp
    Source/Renderer/Generic/Types/Mesh.h
    Source/Renderer/Generic/Types/Pipeline.cpp
    Source/Renderer/Generic/Types/Pipeline.h
    Source/Renderer/Generic/Types/Shader.cpp
    Source/Renderer/Generic/Types/Shader.h
    Source/Renderer/Generic/Types/Texture.cpp
    Source/Renderer/Generic/Types/Texture.h

    # Reflections
    Source/Reflections/DynamicStruct.h
    Source/Reflections/ReflectedStruct.h
    Source/Reflections/ShaderReflection.h

    # Scene
    Source/Scene/ComponentRegistry.cpp
    Source/Scene/ComponentRegistry.h
    Source/Scene/Components.h
    Source/Scene/Entity.h
    Source/Scene/Scene.cpp
    Source/Scene/Scene.h
    Source/Scene/SceneCamera.cpp

    Source/Scene/SceneCamera.h

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

    # Scripting
    Source/Scripting/ScriptEngine.h
    Source/Scripting/ScriptEngine.cpp
    Source/Scripting/EditorScriptEngine.h
    Source/Scripting/EditorScriptEngine.cpp
    Source/Scripting/RuntimeScriptEngine.h
    Source/Scripting/RuntimeScriptEngine.cpp
    Source/Scripting/ScriptGlue.h
    Source/Scripting/ScriptGlue.cpp
    Source/Scripting/ScriptTypes.h
    Source/Scripting/ScriptTypes.cpp
    Source/Scripting/ScriptUtils.h
    Source/Scripting/ScriptUtils.cpp
    Source/Scripting/ScriptField.h
    Source/Scripting/ScriptFieldHandler.h
    Source/Scripting/ScriptFieldHandler.cpp
    Source/Scripting/ScriptFieldRegistry.h
    Source/Scripting/ScriptFieldRegistry.cpp

    # Serialization (Core - used in both editor and runtime)
    Source/Serialization/BinaryDeserializer.h
    Source/Serialization/BinarySerializer.h
    Source/Serialization/Scene/SceneSerializer.cpp
    Source/Serialization/Scene/SceneSerializer.h

    # Math
    Source/Maths/Maths.cpp
    Source/Maths/Maths.h

    # Utils (Core)
    Source/Utils/PlatformUtils.h
    Source/Utils/StringUtils.h

    # Physics
    Source/Physics/PhysicsWorld.h
    Source/Physics/PhysicsWorld.cpp

    # Debug
    Source/Debug/Instrumentor.h
)

# ============================================================================
# Editor-Specific Source Files (Conditional)
# ============================================================================

if(GRAVIX_BUILD_EDITOR)
    set(GRAVIX_EDITOR_SOURCES
        # Debug (Editor)
        Source/Debug/ProfilerViewer.h
        Source/Debug/ProfilerViewer.cpp

        # Editor UI
        Source/Renderer/ImGuiBuild.cpp
        Source/Renderer/ImGuiRender.cpp
        Source/Renderer/ImGuiRender.h
        Source/Scene/ImGuiHelpers.cpp
        Source/Scene/ImGuiHelpers.h

        # Editor Camera
        Source/Scene/EditorCamera.cpp
        Source/Scene/EditorCamera.h

        # Editor Asset System
        Source/Asset/EditorAssetManager.cpp
        Source/Asset/EditorAssetManager.h
        Source/Asset/AssetPack/PakBuilder.h
        Source/Asset/AssetImporter.cpp
        Source/Asset/AssetImporter.h
        Source/Asset/Importers/SceneImporter.cpp
        Source/Asset/Importers/TextureImporter.cpp
        Source/Asset/Importers/ShaderImporter.cpp
        Source/Asset/Importers/PipelineImporter.cpp
        Source/Asset/Importers/MaterialImporter.cpp

        # Editor Serialization (YAML-based)
        Source/Serialization/Project/ProjectSerializer.cpp

        # Shader Compilation (Editor only - runtime uses pre-compiled)
        Source/Utils/ShaderCompiler.cpp
        Source/Utils/ShaderCompiler.h
        Source/Utils/ShaderCompilerSystem.cpp
        Source/Utils/ShaderCompilerSystem.h
        Source/Utils/ShaderReflector.cpp
        Source/Utils/ShaderReflector.h
        Source/Utils/SlangTypeUtils.cpp
        Source/Utils/SlangTypeUtils.h
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
        Source/Renderer/Vulkan/VulkanCommandImpl.h
        Source/Renderer/Vulkan/VulkanDevice.cpp
        Source/Renderer/Vulkan/VulkanDevice.h
        Source/Renderer/Vulkan/VulkanSwapchain.h
        Source/Renderer/Vulkan/VulkanSwapchain.cpp
        Source/Renderer/Vulkan/VulkanRenderCaps.cpp
        Source/Renderer/Vulkan/VulkanRenderCaps.h

        # Vulkan Types
        Source/Renderer/Vulkan/Types/VulkanFramebuffer.cpp
        Source/Renderer/Vulkan/Types/VulkanFramebuffer.h
        Source/Renderer/Vulkan/Types/VulkanMaterial.cpp
        Source/Renderer/Vulkan/Types/VulkanMaterial.h
        Source/Renderer/Vulkan/Types/VulkanMesh.cpp
        Source/Renderer/Vulkan/Types/VulkanMesh.h
        Source/Renderer/Vulkan/Types/VulkanShader.cpp
        Source/Renderer/Vulkan/Types/VulkanShader.h
        Source/Renderer/Vulkan/Types/VulkanTexture.cpp
        Source/Renderer/Vulkan/Types/VulkanTexture.h

        # Vulkan Utils
        Source/Renderer/Vulkan/Utils/DescriptorWriter.cpp
        Source/Renderer/Vulkan/Utils/DescriptorWriter.h
        Source/Renderer/Vulkan/Utils/VulkanInitializers.cpp
        Source/Renderer/Vulkan/Utils/VulkanInitializers.h
        Source/Renderer/Vulkan/Utils/VulkanTypes.h
        Source/Renderer/Vulkan/Utils/VulkanUtils.cpp
        Source/Renderer/Vulkan/Utils/VulkanUtils.h
        Source/Renderer/Vulkan/Utils/VulkanDeviceInit.cpp
        Source/Renderer/Vulkan/Utils/VulkanDeviceInit.h
        Source/Renderer/Vulkan/Utils/VulkanDescriptorSetup.cpp
        Source/Renderer/Vulkan/Utils/VulkanDescriptorSetup.h
        Source/Renderer/Vulkan/Utils/VulkanCommandSetup.cpp
        Source/Renderer/Vulkan/Utils/VulkanCommandSetup.h
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
