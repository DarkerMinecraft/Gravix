# Gravix Game Engine

<div align="center">

**A modern C++23 game engine built with Vulkan for high-performance graphics rendering**

[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C.svg)](https://en.cppreference.com/w/cpp/23)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.3-AC162C.svg)](https://www.vulkan.org/)
[![.NET](https://img.shields.io/badge/.NET-9.0-512BD4.svg)](https://dotnet.microsoft.com/)

</div>

---

## Overview

Gravix is a modern game engine designed for high-performance graphics rendering and game development. Built with cutting-edge C++23 features and the Vulkan graphics API, it provides a robust foundation for creating modern games and interactive applications.

### Key Features

- **Modern C++23** - Leverages the latest C++ standard for performance, safety, and developer experience
- **Vulkan Rendering** - High-performance graphics API with explicit GPU control
- **Entity Component System** - Powered by EnTT for efficient data-oriented design
- **C# Scripting** - .NET 9.0 integration for game logic and rapid prototyping
- **Advanced Task System** - Multi-threaded architecture using enkiTS
- **Slang Shader Language** - Modern shader compiler with cross-platform support
- **ImGui Editor** - Orbit editor with real-time scene editing and asset management
- **Asset Pipeline** - Async loading with project-based asset management
- **Serialization** - Binary and YAML serialization for scenes, materials, and projects
- **2D Physics** - Integrated Box2D for 2D physics simulation

### Platform Support

**⚠️ Currently Windows Only**

Gravix currently supports Windows 10/11. The architecture is designed for future multi-platform support (Linux/macOS).

---

## Quick Start

### Prerequisites

Before building Gravix, ensure you have the following installed:

| Requirement | Version | Notes |
|------------|---------|-------|
| **Windows** | 10/11 | Required |
| **Visual Studio** | 2019+ | With C++ desktop development workload |
| **CMake** | 3.20+ | [Download](https://cmake.org/download/) |
| **Vulkan SDK** | Latest | [Download](https://vulkan.lunarg.com/) |
| **.NET SDK** | 9.0+ | [Download](https://dotnet.microsoft.com/download/dotnet/9.0) (for scripting support) |
| **Python** | 3.7+ | For release build scripts |
| **Ninja** | Latest | Optional, for faster builds |

### Installation

1. **Clone the repository with submodules:**
   ```bash
   git clone --recursive https://github.com/DarkerMinecraft/Gravix.git
   cd Gravix
   ```

2. **Run the interactive build script:**
   ```bash
   Scripts\build.bat
   ```

   The script will guide you through:
   - Build type selection (Debug/Release/RelWithDebInfo)
   - Feature configuration (Vulkan, Editor, Scripting)
   - Generator selection (Ninja/Visual Studio)

3. **Launch the editor:**
   ```bash
   build\Debug\bin\Orbit.exe
   ```

---

## Build Instructions

### Interactive Build (Recommended)

The interactive build script provides a user-friendly way to configure and build the engine:

```bash
Scripts\build.bat
```

**Build Configuration Options:**

1. **Build Type**
   - `Debug` - Full debug symbols, no optimization
   - `Release` - Full optimization, no debug info
   - `RelWithDebInfo` - Optimized with debug symbols

2. **Features**
   - `Vulkan Renderer` - Enable/disable Vulkan graphics backend (default: ON)
   - `Orbit Editor` - Include the editor application (default: ON)
   - `C# Scripting` - Enable .NET scripting support (default: ON, requires .NET SDK)

3. **Generator**
   - `Ninja` - Fast, single-configuration builds
   - `Visual Studio 2022` - Full IDE integration
   - `Visual Studio 2019` - Legacy IDE support

### Manual CMake Configuration

For advanced users, you can configure CMake manually:

```bash
# Example: Debug build with all features using Ninja
cmake -S . -B build/Debug -G Ninja ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DGRAVIX_USE_VULKAN=ON ^
    -DGRAVIX_BUILD_EDITOR=ON ^
    -DGRAVIX_BUILD_SCRIPTING=ON

# Build the project
cmake --build build/Debug --config Debug --parallel
```

**CMake Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `GRAVIX_USE_VULKAN` | ON | Enable Vulkan rendering backend |
| `GRAVIX_BUILD_EDITOR` | ON | Build Orbit editor application |
| `GRAVIX_BUILD_SCRIPTING` | ON | Build C# scripting support |

### Release Build & Packaging

To create a distributable release package:

```bash
Scripts\build_release.bat
```

This will:
1. Check for Visual Studio and .NET SDK
2. Build the Release configuration using Ninja
3. Collect executables, assets, and runtime dependencies
4. Create a timestamped ZIP package in `dist/`

The release script is powered by `Scripts\build_release.py` and can be run directly:

```bash
python Scripts\build_release.py --clean  # Clean build
python Scripts\build_release.py          # Incremental build
```

### Build Output

Build artifacts are located in:
```
build\[BuildType]\
├── bin\                  # Executables and DLLs
├── Orbit\                # Editor output
│   ├── Orbit.exe
│   ├── Assets\           # Engine assets
│   └── EditorAssets\     # Editor-specific assets
└── Gravix-ScriptCore\    # C# scripting assemblies
```

---

## Project Structure

```
Gravix/
├── Gravix/                    # Core engine library (static)
│   ├── Core/                  # Application framework, logging, UUID
│   ├── Renderer/              # Graphics system
│   │   ├── Generic/           # Platform-agnostic rendering API
│   │   └── Vulkan/            # Vulkan implementation
│   ├── Scene/                 # Entity Component System
│   ├── Asset/                 # Asset management and loading
│   ├── Scripting/             # C# scripting engine
│   ├── Serialization/         # Binary/YAML serialization
│   ├── Project/               # Project management
│   ├── Events/                # Event system
│   └── Reflections/           # Runtime type reflection
│
├── Orbit/                     # Editor application
│   ├── App.cpp                # Entry point
│   ├── AppLayer/              # Main editor layer
│   └── Panels/                # Editor UI panels
│       ├── SceneHierarchyPanel.h
│       ├── InspectorPanel.h
│       ├── ViewportPanel.h
│       └── ContentBrowserPanel.h
│
├── Gravix-ScriptCore/         # .NET 9.0 scripting core
│   └── Main.cs                # C#/C++ interop
│
├── ThirdParties/              # External dependencies
│   ├── ImGui/                 # UI framework
│   ├── Slang/                 # Shader compiler
│   ├── enkiTS/                # Task scheduler
│   ├── yaml-cpp/              # YAML parser
│   ├── entt/                  # ECS library
│   └── ...
│
├── Assets/                    # Engine assets
├── EditorAssets/              # Editor-specific assets
├── Scripts/                   # Build scripts
│   ├── build.bat              # Interactive build
│   ├── build_release.bat      # Release packaging
│   └── build_release.py       # Python build automation
│
├── CMakeLists.txt             # Root CMake configuration
├── README.md                  # This file
└── CLAUDE.md                  # AI assistant project guide
```

---

## Architecture Overview

### Core Modules

#### 1. **Core Engine (Gravix)**

The heart of the engine, providing fundamental systems:

- **Application & Window** - Main loop, event handling, window management
- **Layer System** - Stackable update/render layers for modular architecture
- **Scheduler** - Multi-threaded task system using enkiTS
- **Logging** - Comprehensive logging via spdlog
- **Smart Pointers** - Type aliases (`Ref<T>`, `Scope<T>`, `Weak<T>`)

#### 2. **Rendering System**

Two-tier architecture for platform abstraction:

- **Generic Layer** - Abstract rendering API (Command, Device, Material, Mesh)
- **Vulkan Layer** - Concrete Vulkan implementation
- **Renderer2D** - Optimized 2D sprite rendering
- **Camera System** - Orthographic, editor, and scene cameras
- **Material System** - Shader reflection with dynamic uniform binding

**Key Pattern:** Materials compile shaders and extract layout metadata, enabling type-safe dynamic uniform updates.

#### 3. **Entity Component System**

Data-oriented design using EnTT:

- **Scene** - Entity container and registry
- **Entity** - Lightweight entity handle
- **Components** - Transform, Sprite, Camera, Rigid Body, etc.
- **Systems** - Update and render systems

#### 4. **Asset Management**

Async, project-based asset pipeline:

- **Asset Types** - Scene, Texture2D, Material, Script
- **EditorAssetManager** - Development-time disk loading
- **RuntimeAssetManager** - Packaged asset bundles
- **Importers** - Texture, Scene, and custom asset importers
- **Async Loading** - Background loading with completion callbacks

#### 5. **Scripting Engine**

.NET 9.0 integration for game logic:

- **ScriptEngine** - C# runtime hosting via nethost
- **Assembly Loading** - Dynamic assembly loading and hot-reload
- **Function Pointers** - Direct C++/C# interop

#### 6. **Serialization**

Flexible serialization framework:

- **Binary** - Efficient binary serialization with magic headers
- **YAML** - Human-readable configuration files
- **Traits** - Automatic detection of `Serialize()` methods
- **Specialized Serializers** - Scene, Material, Project

### Key Design Patterns

1. **Factory Pattern** - Static `Create()` methods for resource creation
2. **Plugin Architecture** - Layer-based extensibility
3. **Type-Safe Events** - Template-based event dispatch
4. **Descriptor Pattern** - Materials with automatic reflection
5. **Command Recording** - Deferred rendering commands
6. **Double Buffering** - `FRAME_OVERLAP = 2` for smooth rendering

---

## Coding Conventions

### Naming

- **Classes/Structs:** `PascalCase`
- **Functions/Methods:** `PascalCase`
- **Member Variables:** `camelCase` with `m_` prefix
- **Constants:** `UPPER_SNAKE_CASE`
- **Enums:** `PascalCase` with `PascalCase` values

### Memory Management

- **Prefer** `Ref<T>` (shared_ptr) for shared ownership
- **Prefer** `Scope<T>` (unique_ptr) for exclusive ownership
- **Never** use raw `new`/`delete` directly
- **RAII** for all resources

### Threading

- **Main Thread** - Updates, rendering, and UI
- **Worker Threads** - Asset loading, mesh building (via enkiTS)
- **Thread Safety** - Minimal locking, use task-based parallelism

### Example

```cpp
// Create an entity with components
Entity entity = scene->CreateEntity("Player");
auto& transform = entity.AddComponent<TransformComponent>();
auto& sprite = entity.AddComponent<SpriteRendererComponent>();

// Load an asset
auto texture = AssetManager::GetAsset<Texture2D>(textureHandle);

// Render a mesh
Command cmd;
cmd.SetActiveMaterial(material.get());
cmd.BindMesh(mesh);
cmd.DrawIndexed(mesh->GetIndexCount());
```

---

## Editor Usage

### Orbit Editor

The Orbit editor provides a complete development environment:

1. **Scene Hierarchy Panel** - View and organize entities
2. **Inspector Panel** - Edit component properties
3. **Viewport Panel** - Visual scene editing with gizmos
4. **Content Browser** - Asset management and importing
5. **Project Settings** - Configure project properties

### Creating a New Project

1. Launch Orbit
2. File → New Project
3. Choose location and name
4. Configure asset directories

### Working with Assets

- **Import Textures** - Drag & drop images into Content Browser
- **Create Materials** - Right-click → New Material
- **Create Scenes** - File → New Scene

---

## Dependencies

Gravix integrates the following third-party libraries:

| Library | Purpose | License |
|---------|---------|---------|
| **Vulkan** | Graphics API | Apache 2.0 |
| **vk-bootstrap** | Vulkan initialization | MIT |
| **ImGui** | UI framework | MIT |
| **ImGuizmo** | 3D gizmos | MIT |
| **EnTT** | Entity Component System | MIT |
| **Slang** | Shader compiler | Apache 2.0 |
| **enkiTS** | Task system | zlib |
| **GLM** | Math library | MIT |
| **stb_image** | Image loading | Public Domain |
| **yaml-cpp** | YAML parser | MIT |
| **spdlog** | Logging | MIT |
| **Box2D** | 2D physics | MIT |
| **VMA** | Vulkan Memory Allocator | MIT |

---

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Follow the coding conventions outlined above
4. Write clear commit messages
5. Test your changes thoroughly
6. Submit a pull request

### Development Setup

1. Follow the build instructions above
2. Use the Debug build configuration
3. Enable all features for comprehensive testing
4. Run from Visual Studio for integrated debugging

---

## Troubleshooting

### Common Build Issues

**CMake can't find Vulkan SDK:**
- Ensure `VULKAN_SDK` environment variable is set
- Restart your terminal/IDE after installing Vulkan SDK

**.NET scripting fails to build:**
- Verify .NET 9.0 SDK is installed: `dotnet --version`
- Add .NET to your PATH if not found
- Disable scripting if not needed: `-DGRAVIX_BUILD_SCRIPTING=OFF`

**Ninja not found:**
- Install Ninja: `choco install ninja` (via Chocolatey)
- Or use Visual Studio generator instead

**Submodules not initialized:**
```bash
git submodule update --init --recursive
```

---

## Roadmap

- [ ] Linux support
- [ ] macOS support (MoltenVK)
- [ ] 3D physics integration (Jolt)
- [ ] PBR rendering pipeline
- [ ] Animation system
- [ ] Audio system (OpenAL)
- [ ] Networking layer
- [ ] Particle system
- [ ] Visual shader editor

---

## License

This project is currently under development. License information will be provided in future releases.

---

## Acknowledgments

- Built with modern C++23 and Vulkan
- Powered by open-source libraries from the game development community
- Inspired by industry-leading game engines

---

<div align="center">

**Gravix Engine** • Version 1.0.0 • 2025

[Documentation](CLAUDE.md) • [Issues](https://github.com/DarkerMinecraft/Gravix/issues) • [Discussions](https://github.com/DarkerMinecraft/Gravix/discussions)

</div>
