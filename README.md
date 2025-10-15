# Gravix Game Engine

Gravix is a modern C++23 game engine built with Vulkan for high-performance graphics rendering. The engine focuses on providing a robust foundation for game development with advanced rendering capabilities.

## Platform Support

**⚠️ Currently Windows Only**

Gravix currently supports Windows only. Linux and macOS support is planned for future releases.

## Building the Engine

To build Gravix, simply run the setup script from the repository root:

```bash
Scripts/Setup.bat
```

This script will:
- Set up the build environment
- Configure CMake with the appropriate settings
- Handle dependencies and third-party libraries
- Generate the Visual Studio solution files

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later with C++ support
- CMake 3.20 or higher
- Vulkan SDK (handled automatically by setup script)
- Python 3.7+ (for build scripts)

## Engine Features

- **Modern C++23**: Built with the latest C++ standard for performance and developer experience
- **Vulkan Rendering**: High-performance graphics API for modern GPUs
- **Modular Architecture**: Separated into core engine (Gravix) and editor/tools (Orbit)
- **Third-Party Integration**: Includes ImGui, Slang shader language, and enkiTS task system
- **Cross-Platform Ready**: Architecture designed for future multi-platform support

## Project Structure

```
Gravix/
├── Gravix/          # Core engine module
├── Orbit/           # Editor and development tools
├── Scripts/         # Build and setup scripts
├── ThirdParties/    # External dependencies
└── Assets/          # Engine assets and resources
```

## Getting Started

1. Clone the repository with submodules:
   ```bash
   git clone --recursive https://github.com/DarkerMinecraft/Gravix.git
   ```

2. Run the setup script:
   ```bash
   cd Gravix
   Scripts\Setup.bat
   ```

3. Open the generated Visual Studio solution and build the project.

## License

This project is currently under development. License information will be provided in future releases.