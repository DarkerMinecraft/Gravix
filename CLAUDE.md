# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Gravix is a modern C++23 game engine built with Vulkan for high-performance graphics rendering. The engine focuses on providing a robust foundation for game development with advanced rendering capabilities.

**Platform Support**: Currently Windows-only, but architecture is designed for future multi-platform support (Linux/macOS).

**Key Technologies**: C++23, Vulkan, .NET 9.0 scripting, EnTT ECS, Slang shader compiler, ImGui, enkiTS task system, YAML serialization

---

## Build Commands

### Prerequisites
- Windows 10/11
- Visual Studio 2019+ with C++ support
- CMake 3.20+
- Vulkan SDK
- Python 3.7+ (for release builds)
- .NET 9.0 SDK (for scripting support)

### Interactive Build (Recommended)
```bash
# Interactive build script with configuration options
Scripts\build.bat
```
The script will prompt for:
- Build type (Debug/Release/RelWithDebInfo)
- Vulkan renderer (ON/OFF)
- Orbit editor (ON/OFF)
- C# scripting support (ON/OFF)
- Generator (Ninja/Visual Studio 2022/2019)
- Clean previous build (y/n)

### Release Build & Packaging (Recommended)
```bash
# Create optimized release package with auto DLL collection
Scripts\build_release.bat
```
The modular Python release builder:
- Configures CMake with full optimizations (O2, LTCG, inlining)
- Builds only: Gravix-ScriptCore, Gravix, Orbit (skips unnecessary targets)
- Auto-detects and collects required DLLs (Slang, Mono, etc.)
- Creates versioned ZIP: `dist/Orbit_v{VERSION}_{TIMESTAMP}.zip`
- Visual Studio-like build output showing file compilation progress
- Uses VERSION file in root for package naming

Advanced options:
```bash
# Skip CMake configuration (reuse existing)
Scripts\build_release.bat --skip-configure

# Control parallel jobs and verbosity
Scripts\build_release.bat --jobs 16 --verbosity 2

# Custom directories
Scripts\build_release.bat --build-dir "C:/custom/build" --output-dir "C:/output"
```

See `Scripts/OrbitRelease/README.md` for detailed documentation.

### Manual CMake Configuration
```bash
# Example: Debug build with all features
cmake -S . -B build/Debug -G Ninja ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DGRAVIX_USE_VULKAN=ON ^
    -DGRAVIX_BUILD_EDITOR=ON ^
    -DGRAVIX_BUILD_SCRIPTING=ON

cmake --build build/Debug --config Debug --parallel
```

**Build Output**: `build/[BuildType]/bin/`

### CMake Options
- `GRAVIX_USE_VULKAN` - Enable Vulkan renderer (default: ON)
- `GRAVIX_BUILD_EDITOR` - Build Orbit editor (default: ON)
- `GRAVIX_BUILD_SCRIPTING` - Build C# scripting support (default: ON)

---

## Main Modules

### 1. Gravix Core Engine (Static Library)

#### Core/ - Application & Engine Loop
- Application.h - Engine loop, window, layer stack
- Layer.h - Abstract base for stackable systems
- Window.h - Window abstraction
- Scheduler.h - Multi-threaded task system (enkiTS)
- UUID.h - Unique identifiers (used for assets)
- Log.h - Logging (spdlog)
- Core.h - Smart pointer aliases and macros

Smart Pointers:
- Ref<T> = std::shared_ptr<T>
- Scope<T> = std::unique_ptr<T>
- Weak<T> = std::weak_ptr<T>

#### Renderer/ - Graphics System
Generic Abstraction (Generic/):
- Command.h - Render command recording
- Device.h - Abstract GPU device
- Renderer2D.h - 2D rendering
- Camera classes - Orthographic, Editor, Scene cameras
- Specification.h - Rendering enums (topology, cull, blend)

Vulkan Implementation (Vulkan/):
- VulkanDevice.h - Vulkan instance, device, swapchain
- VulkanCommandImpl.h - Vulkan command buffers
- Types/ - VulkanTexture, VulkanFramebuffer, VulkanMaterial, VulkanMesh
- Utils/ - Vulkan helpers, descriptor writers, initializers

Rendering Types (Generic/Types/):
- Texture.h, Texture2D.h - Texture interface
- Framebuffer.h - Offscreen render target
- Material.h - Shader with dynamic reflection
- Mesh.h - Vertex/index buffers

Key Pattern:
- Materials compile shaders and extract layout metadata
- DynamicStruct mirrors GPU data layout
- Bindless descriptor sets for efficiency
- FRAME_OVERLAP = 2 for double-buffering

#### Asset/ - Asset Management

Core Classes:
- Asset.h - Base class (Scene, Texture2D, Material, Script)
- AssetHandle = UUID
- AssetManagerBase.h - Interface
- EditorAssetManager.h - Disk loading
- RuntimeAssetManager.h - Packaged assets

Importers:
- TextureImporter.h - Image loading (stb_image)
- SceneImporter.h - YAML scene loading
- Custom importers inherit AssetImporter

Async System:
- AsyncLoadRequest.h - Load operations
- Two-phase: CPU load -> GPU completion queue
- Scheduler distributes on thread pool

#### Project/ - Project Management
- Project.h - Singleton managing active project
- ProjectConfig - Name, asset/library dirs, script path
- Project::GetActive() - Static access
- Creates appropriate asset manager

#### Scene/ - ECS System
- Scene.h - Entity container (EnTT registry)
- Entity.h - Entity wrapper
- Components.h - Standard components
  - TagComponent: Name, UUID
  - TransformComponent: Position, Rotation, Scale
  - SpriteRendererComponent: Color, Texture, Tiling
  - CameraComponent: Projection
- EditorCamera.h, SceneCamera.h - Camera systems

#### Serialization/ - Data Persistence
- BinarySerializer.h, BinaryDeserializer.h - Generic binary I/O
- Traits detect Serialize() methods
- Automatic container support
- GRAVIXBN magic header
- Specialized: SceneSerializer, MaterialSerializer, ProjectSerializer

#### Scripting/ - .NET Integration
- ScriptEngine.h - C# runtime hosting via nethost
- Loads .NET assemblies
- Function pointer retrieval
- Assembly hot-reload

#### Reflections/ - Type Introspection
- ReflectedStruct.h - Metadata (name, fields, offsets)
- DynamicStruct.h - Runtime instances with field access
- Used for vertex layouts, push constants

#### Events/ - Event System
- Event.h - Type-safe dispatch
- EventDispatcher - Template dispatch
- EventType enum: WindowClose, KeyPressed, etc.
- Derived: KeyEvents, MouseEvents, WindowEvents

---

### 2. Orbit Editor (Executable)

- App.cpp - Entry point
- AppLayer.h/cpp - Main editor logic
- Panels/:
  - SceneHierarchyPanel - Entity tree
  - InspectorPanel - Component editor
  - ViewportPanel - Scene view
  - ContentBrowserPanel - Asset browser
  - ProjectSettingsPanel - Config

Responsibilities:
- Scene management (create, load, save)
- Asset browsing/importing
- Entity/component editing
- Project lifecycle

---

### 3. Gravix-ScriptCore (.NET 9.0)

- Main.cs - Example C#/C++ interop
- Compiled to DLL
- Loaded by ScriptEngine
- Game logic scripting

---

## Build System (CMake)

Root CMakeLists.txt:
- C++23 standard
- Platform detection (Windows, Linux, macOS)
- Third-party configuration
- Adds: Gravix-ScriptCore, Gravix, Orbit

Third-Parties:
VkBootstrap, enkiTS, ImGui, Slang, yaml-cpp, ImGuizmo, glm, spdlog, stb, entt, VMA

Gravix (Static):
- Precompiled: pch.h
- Links: Vulkan, vk-bootstrap, enkiTS, ImGui, ImGuizmo, slang, yaml-cpp, nethost

Orbit (Executable):
- Links Gravix
- Asset copy targets
- UTF-8 MSVC flag

Gravix-ScriptCore:
- Custom target: dotnet build
- Output: CMAKE_BINARY_DIR/Gravix-ScriptCore

---

## Key Architectural Patterns

1. **Plugin Architecture** - Layer-based system
2. **Factory Pattern** - Static Create() methods
3. **Type-Safe Event System** - Template dispatch
4. **Descriptor Pattern** - Materials with reflection
5. **ECS** - Entity Component System (EnTT)
6. **Async Multithreading** - enkiTS task scheduler
7. **Project Abstraction** - Singleton with pluggable managers
8. **Generic/Concrete Split** - API abstraction with Vulkan impl

---

## Conventions

Naming:
- Classes: PascalCase
- Functions: PascalCase
- Members: camelCase with m_ prefix
- Constants: UPPER_SNAKE_CASE

Memory:
- Ref<T> for shared ownership
- Scope<T> for exclusive ownership
- Never use new/delete directly

Threading:
- enkiTS for async work
- Main thread updates/renders
- Asset loading on thread pool

Serialization:
- Serialize(BinarySerializer&) for custom types
- YAML for config
- GRAVIXBN magic header

---

## Module Dependencies

Orbit -> Gravix -> [Vulkan, vk-bootstrap, enkiTS, ImGui, ImGuizmo, Slang, yaml-cpp, glm, spdlog, stb, entt, VMA, nethost]
                         ^
                         |
                    Gravix-ScriptCore (.NET)

---

## Quick Reference

Load Asset:
auto texture = AssetManager::GetAsset<Texture2D>(handle);

Create Entity:
Entity entity = scene->CreateEntity("MyEntity");
auto& t = entity.AddComponent<TransformComponent>();

Render Mesh:
Command cmd;
cmd.SetActiveMaterial(mat.get());
cmd.BindMesh(mesh);
cmd.DrawIndexed(mesh->GetIndexCount());

Serialize:
BinarySerializer s(VER);
s.Write(data);
s.WriteToFile("file.dat");

Get Project:
auto proj = Project::GetActive();
auto& mgr = proj->GetAssetManager();

---

Version: 1.0.0 | C++23 | Windows 10+
Updated: 2025-11-08
- I will handle the cmake stuff and building stuff
- The working directory is /out/build/Debug-x64/Orbit please refer to that when ask about timings for anything
- Never build the engine
- I am using mono now