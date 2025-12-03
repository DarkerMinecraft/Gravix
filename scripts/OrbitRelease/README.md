# Orbit Release Builder

Modular Python build system for creating optimized release builds of Orbit with automatic DLL collection and packaging.

## Features

- **Modular Architecture**: Clean separation of concerns with dedicated modules for configuration, building, and packaging
- **Automatic DLL Collection**: Automatically detects and collects required runtime DLLs (Mono, etc.)
- **Visual Studio-Like Output**: Ninja build progress displayed in a familiar Visual Studio format
- **Full Optimizations**: Release builds with all compiler optimizations enabled
- **Automatic Packaging**: Creates timestamped ZIP archives with version information
- **Selective Building**: Only builds Gravix, Gravix-ScriptCore, and Orbit (no unnecessary targets)
- **ImGui Configuration**: Includes pre-configured imgui.ini with optimized editor layout

## Requirements

- Python 3.7+
- CMake 3.20+
- Ninja build system
- Visual Studio 2019+ (for MSVC compiler)
- .NET 9.0 SDK (for Gravix-ScriptCore)

## Quick Start

### Basic Usage

```bash
# Navigate to the build script directory
cd Scripts/OrbitRelease

# Run the build
python build.py
```

This will:
1. Configure CMake with Release optimizations
2. Build Gravix-ScriptCore, Gravix, and Orbit
3. Collect all required DLLs
4. Package everything into `dist/Orbit_v{VERSION}_{TIMESTAMP}.zip`

### Advanced Options

```bash
# Skip CMake configuration (use existing)
python build.py --skip-configure

# Skip build step (useful for re-packaging)
python build.py --skip-build

# Custom build directory
python build.py --build-dir "C:/custom/build"

# Custom output directory
python build.py --output-dir "C:/custom/output"

# Control parallel jobs
python build.py --jobs 8

# Control verbosity (0=quiet, 1=default, 2=verbose)
python build.py --verbosity 2
```

## Module Overview

### `config.py` - Build Configuration

Contains all configuration classes:

- **`BuildConfig`**: Main build settings (directories, targets, CMake options)
- **`DLLConfig`**: DLL collection settings (search paths, patterns, exclusions)
- **`CompilerConfig`**: Compiler optimization flags (MSVC, Clang, GCC)

Example customization:

```python
from config import BuildConfig

config = BuildConfig.create_default()
config.parallel_jobs = 16  # Use 16 cores
config.ninja_verbosity = 2  # Full verbose output
```

### `cmake_builder.py` - CMake Build System

Handles CMake configuration and Ninja build execution:

- Configures CMake with release optimizations
- Executes Ninja builds with Visual Studio-like progress display
- Manages multi-target builds
- Tracks build time and statistics

### `dll_collector.py` - Dependency Collection

Automatically finds and collects required DLLs:

- Uses `dumpbin` for dependency analysis (if available)
- Searches known directories for common DLLs
- Filters out Windows system DLLs
- Generates dependency manifests

Configured DLLs collected:
- `mono-2.0-sgen.dll` - Mono runtime (if using Mono)

### `build.py` - Main Orchestrator

Main entry point that coordinates the entire build process:

1. CMake configuration with optimizations
2. Multi-target build (ScriptCore → Gravix → Orbit)
3. DLL dependency collection
4. Release packaging and ZIP creation

## Output Structure

After a successful build, you'll find:

```
dist/
└── Orbit_v1.0.0_20251203_143022/
    ├── Orbit.exe
    ├── Gravix-ScriptCore.dll
    ├── imgui.ini
    ├── mono-2.0-sgen.dll (if using Mono)
    ├── Assets/
    │   └── (all asset files)
    └── dll_manifest.txt
```

And a ZIP archive: `Orbit_v1.0.0_20251203_143022.zip`

## Optimization Flags

### MSVC (Windows)

The following optimizations are automatically applied:

- `/O2` - Maximum optimization
- `/Ob2` - Inline function expansion
- `/Oi` - Enable intrinsic functions
- `/Ot` - Favor fast code over small code
- `/GL` - Whole program optimization
- `/GF` - String pooling
- `/Gy` - Function-level linking
- `/fp:fast` - Fast floating-point operations
- `/LTCG` - Link-time code generation

### Clang/GCC (Future)

- `-O3` - Maximum optimization
- `-march=native` - CPU-specific optimizations
- `-flto` - Link-time optimization

## Versioning

The build system reads the version from the `VERSION` file in the project root. Update this file to change the version number:

```
C:\Dev\Gravix\VERSION
```

Format: `MAJOR.MINOR.PATCH` (e.g., `1.0.0`)

## Customization

### Adding New DLLs

Edit `config.py`:

```python
self.include_patterns = [
    "slang.dll",
    "your-new-dll.dll",  # Add here
]
```

### Adding New Search Directories

Edit `config.py`:

```python
self.search_dirs = [
    "ThirdParties/mono/bin/Release",
    "path/to/your/dlls",  # Add here
]
```

### Changing Build Targets

Edit `config.py`:

```python
self.targets = ["Gravix-ScriptCore", "Gravix", "Orbit", "YourNewTarget"]
```

### Adjusting Compiler Flags

Edit `config.py` in the `CompilerConfig` class:

```python
self.msvc_flags = [
    "/O2",
    "/YourCustomFlag",  # Add here
]
```

## Troubleshooting

### "dumpbin not found"

The build system will fall back to manual DLL search. To use dumpbin:
1. Run from a Visual Studio Developer Command Prompt, or
2. Add Visual Studio's `bin` directory to your PATH

### Missing DLLs in Package

Check `dll_manifest.txt` in the package to see what was collected. Add missing DLLs to `config.py`:

```python
self.include_patterns = [
    "missing-dll.dll",
]
```

### Build Fails

1. Ensure all prerequisites are installed
2. Try with `--verbosity 2` for detailed output
3. Check CMake configuration with `--skip-build`

### Slow Builds

1. Increase parallel jobs: `--jobs 16`
2. Use an SSD for build directory
3. Disable antivirus for build directory (temporarily)

## Architecture

```
OrbitReleaseBuilder (build.py)
    ├── BuildConfig (config.py)
    ├── CMakeBuilder (cmake_builder.py)
    │   ├── configure() - CMake setup
    │   └── build() - Ninja execution
    ├── DLLCollector (dll_collector.py)
    │   ├── collect_dlls() - Find dependencies
    │   └── copy_dlls() - Copy to package
    └── Package & ZIP creation
```

## Examples

### CI/CD Integration

```bash
# Non-interactive build for CI
python build.py --jobs 8 --verbosity 1

# Upload artifact
upload-artifact dist/Orbit_v*.zip
```

### Development Workflow

```bash
# Initial build
python build.py

# Rebuild after changes (skip configure)
python build.py --skip-configure

# Re-package without building
python build.py --skip-configure --skip-build
```

### Custom Configuration

```python
from build import OrbitReleaseBuilder
from config import BuildConfig

config = BuildConfig.create_default()
config.build_dir = Path("C:/MyCustomBuild")
config.parallel_jobs = 32

builder = OrbitReleaseBuilder(config)
builder.build()
```

## License

Part of the Gravix Engine project.
