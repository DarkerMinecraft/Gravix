#!/usr/bin/env python3
"""
Gravix Engine - Release Build and Distribution Packager

This script automates the process of:
1. Building the Release configuration
2. Collecting all required files (executable, assets, DLLs)
3. Creating a distribution-ready ZIP package
"""

import os
import sys
import shutil
import subprocess
import platform
import zipfile
from pathlib import Path
from datetime import datetime


class Colors:
    """ANSI color codes for terminal output"""
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def print_header(message):
    """Print a formatted header message"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{message:^70}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'='*70}{Colors.RESET}\n")


def print_step(message):
    """Print a step message"""
    print(f"{Colors.BOLD}{Colors.BLUE}▶ {message}{Colors.RESET}")


def print_success(message):
    """Print a success message"""
    print(f"{Colors.GREEN}✓ {message}{Colors.RESET}")


def print_warning(message):
    """Print a warning message"""
    print(f"{Colors.YELLOW}⚠ {message}{Colors.RESET}")


def print_error(message):
    """Print an error message"""
    print(f"{Colors.RED}✗ {message}{Colors.RESET}")


class GravixBuilder:
    """Main builder class for Gravix Engine"""

    def __init__(self):
        self.root_dir = Path(__file__).parent.parent.absolute()
        self.build_type = "Release"
        self.system = platform.system()

        # Use consistent build directory across all platforms
        self.build_dir = self.root_dir / "build" / "Release"

        # Distribution output directory
        self.dist_dir = self.root_dir / "dist"
        self.package_name = f"Gravix-{datetime.now().strftime('%Y%m%d-%H%M%S')}"
        self.package_dir = self.dist_dir / self.package_name

        # Visual Studio environment variables (set during VS setup)
        self.vs_env = None

        # .NET SDK check
        self.dotnet_available = False
        self.dotnet_version = None

    def setup_visual_studio_env(self):
        """Set up Visual Studio development environment on Windows"""
        if self.system != "Windows":
            return True

        print_step("Setting up Visual Studio development environment...")

        # Common Visual Studio installation paths
        vs_paths = [
            r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
            r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
        ]

        vcvarsall_path = None
        for path in vs_paths:
            if os.path.exists(path):
                vcvarsall_path = path
                break

        if not vcvarsall_path:
            # Try using vswhere to find Visual Studio
            try:
                vswhere_path = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
                if os.path.exists(vswhere_path):
                    result = subprocess.run(
                        [vswhere_path, "-latest", "-property", "installationPath"],
                        capture_output=True,
                        text=True,
                        check=True
                    )
                    vs_install_path = result.stdout.strip()
                    if vs_install_path:
                        vcvarsall_path = os.path.join(vs_install_path, r"VC\Auxiliary\Build\vcvarsall.bat")
            except Exception:
                pass

        if not vcvarsall_path or not os.path.exists(vcvarsall_path):
            print_error("Visual Studio installation not found!")
            print_error("Please install Visual Studio 2019 or 2022 with C++ development tools")
            return False

        print_success(f"Found Visual Studio at: {vcvarsall_path}")

        # Run vcvarsall.bat and capture the environment
        print_step("Loading Visual Studio environment variables...")

        # Use a batch script to capture environment after running vcvarsall
        temp_batch = self.root_dir / "temp_env.bat"
        try:
            with open(temp_batch, 'w') as f:
                f.write(f'@echo off\n')
                f.write(f'call "{vcvarsall_path}" x64\n')
                f.write(f'set\n')

            result = subprocess.run(
                [str(temp_batch)],
                capture_output=True,
                text=True,
                shell=True
            )

            # Parse environment variables from output
            self.vs_env = os.environ.copy()
            for line in result.stdout.splitlines():
                if '=' in line:
                    key, _, value = line.partition('=')
                    self.vs_env[key] = value

            print_success("Visual Studio environment loaded successfully")
            return True

        except Exception as e:
            print_error(f"Failed to set up Visual Studio environment: {e}")
            return False
        finally:
            # Clean up temp file
            if temp_batch.exists():
                temp_batch.unlink()

    def check_dotnet_sdk(self):
        """Check if .NET SDK is installed and available"""
        print_step("Checking for .NET SDK...")

        try:
            result = subprocess.run(
                ["dotnet", "--version"],
                capture_output=True,
                text=True,
                check=True
            )
            self.dotnet_version = result.stdout.strip()
            self.dotnet_available = True
            print_success(f"Found .NET SDK version {self.dotnet_version}")

            # Check if version is 9.0 or higher
            try:
                major_version = int(self.dotnet_version.split('.')[0])
                if major_version < 9:
                    print_warning(f".NET SDK {self.dotnet_version} found, but version 9.0+ recommended for scripting support")
            except (ValueError, IndexError):
                pass

            return True

        except (subprocess.CalledProcessError, FileNotFoundError):
            self.dotnet_available = False
            print_warning(".NET SDK not found")
            print_warning("Scripting support will not be available")
            print_warning("Download .NET 9.0 SDK from: https://dotnet.microsoft.com/download/dotnet/9.0")
            return False

    def clean_build(self):
        """Clean previous build artifacts"""
        print_step("Cleaning previous build artifacts...")

        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)
            print_success(f"Removed {self.build_dir}")

        if self.dist_dir.exists():
            shutil.rmtree(self.dist_dir)
            print_success(f"Removed {self.dist_dir}")

    def configure_cmake(self):
        """Configure CMake for Release build"""
        print_step("Configuring CMake for Release build...")

        self.build_dir.mkdir(parents=True, exist_ok=True)

        cmake_args = [
            "cmake",
            "-S", str(self.root_dir),
            "-B", str(self.build_dir),
            f"-DCMAKE_BUILD_TYPE={self.build_type}",
        ]

        # Add platform-specific generator
        if self.system == "Windows":
            cmake_args.extend(["-G", "Ninja"])
        elif self.system == "Linux":
            cmake_args.extend(["-G", "Ninja"])

        try:
            result = subprocess.run(
                cmake_args,
                cwd=self.root_dir,
                check=True,
                capture_output=True,
                text=True,
                env=self.vs_env if self.vs_env else None
            )
            print_success("CMake configuration completed")
            return True
        except subprocess.CalledProcessError as e:
            print_error(f"CMake configuration failed: {e}")
            print(e.stderr)
            return False

    def build_project(self):
        """Build the project using CMake"""
        print_step("Building project in Release mode...")

        build_args = [
            "cmake",
            "--build", str(self.build_dir),
            "--config", self.build_type,
            "--parallel"
        ]

        try:
            result = subprocess.run(
                build_args,
                cwd=self.root_dir,
                check=True,
                capture_output=True,
                text=True,
                env=self.vs_env if self.vs_env else None
            )
            print_success("Build completed successfully")
            return True
        except subprocess.CalledProcessError as e:
            print_error(f"Build failed: {e}")
            print(e.stderr)
            return False

    def collect_files(self):
        """Collect all files needed for distribution"""
        print_step("Collecting distribution files...")

        # Create package directory
        self.package_dir.mkdir(parents=True, exist_ok=True)

        # Determine the Orbit output directory
        if self.system == "Windows":
            orbit_output_dir = self.build_dir / "Orbit"
            executable_name = "Orbit.exe"
        elif self.system == "Darwin":  # macOS
            orbit_output_dir = self.build_dir / "Orbit"
            executable_name = "Orbit"
        else:  # Linux
            orbit_output_dir = self.build_dir / "Orbit"
            executable_name = "Orbit"

        # Copy executable
        print_step(f"Copying {executable_name}...")
        executable_src = orbit_output_dir / executable_name
        executable_dst = self.package_dir / executable_name

        if executable_src.exists():
            shutil.copy2(executable_src, executable_dst)
            print_success(f"Copied {executable_name}")
        else:
            print_error(f"Executable not found: {executable_src}")
            return False

        # Copy Assets directory
        print_step("Copying Assets...")
        assets_src = orbit_output_dir / "Assets"
        assets_dst = self.package_dir / "Assets"

        if assets_src.exists():
            shutil.copytree(assets_src, assets_dst)
            print_success("Copied Assets directory")
        else:
            print_warning("Assets directory not found, skipping...")

        # Copy EditorAssets directory
        print_step("Copying EditorAssets...")
        editor_assets_src = orbit_output_dir / "EditorAssets"
        editor_assets_dst = self.package_dir / "EditorAssets"

        if editor_assets_src.exists():
            shutil.copytree(editor_assets_src, editor_assets_dst)
            print_success("Copied EditorAssets directory")
        else:
            print_warning("EditorAssets directory not found, skipping...")

        # Copy DLL files (Windows) or .so files (Linux)
        print_step("Copying runtime libraries...")

        dll_patterns = []
        if self.system == "Windows":
            dll_patterns = ["*.dll", "*.runtimeconfig.json"]
        elif self.system == "Linux":
            dll_patterns = ["*.so", "*.runtimeconfig.json"]
        elif self.system == "Darwin":
            dll_patterns = ["*.dylib", "*.runtimeconfig.json"]

        copied_count = 0
        for pattern in dll_patterns:
            for dll_file in orbit_output_dir.glob(pattern):
                shutil.copy2(dll_file, self.package_dir / dll_file.name)
                copied_count += 1

        print_success(f"Copied {copied_count} runtime library files")

        # Create a README for distribution
        print_step("Creating README...")
        readme_content = f"""
Gravix Engine - Distribution Package
=====================================

Build Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
Platform: {self.system}
Configuration: {self.build_type}

How to Run:
-----------
1. Extract this package to a directory of your choice
2. Run the Orbit executable to start the editor
3. Create a new project or open an existing one

Directory Structure:
--------------------
- {executable_name}       : Main editor executable
- Assets/                 : Engine core assets
- EditorAssets/           : Editor-specific assets
- *.dll / *.so / *.dylib  : Runtime libraries

Support:
--------
For issues and support, visit: https://github.com/DarkerMinecraft/Gravix

(c) {datetime.now().year} Gravix Engine
"""

        readme_path = self.package_dir / "README.txt"
        readme_path.write_text(readme_content.strip())
        print_success("Created README.txt")

        return True

    def create_zip(self):
        """Create a ZIP archive of the distribution"""
        print_step("Creating distribution ZIP archive...")

        zip_path = self.dist_dir / f"{self.package_name}.zip"

        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for file_path in self.package_dir.rglob('*'):
                if file_path.is_file():
                    arcname = file_path.relative_to(self.dist_dir)
                    zipf.write(file_path, arcname)

        print_success(f"Created {zip_path.name}")

        # Print size information
        zip_size = zip_path.stat().st_size / (1024 * 1024)  # Convert to MB
        print_success(f"Package size: {zip_size:.2f} MB")

        return zip_path

    def run(self, clean=False):
        """Run the complete build process"""
        print_header("Gravix Engine - Release Build System")

        print(f"{Colors.BOLD}Configuration:{Colors.RESET}")
        print(f"  Platform: {self.system}")
        print(f"  Build Type: {self.build_type}")
        print(f"  Build Directory: {self.build_dir}")
        print(f"  Output Package: {self.package_name}")
        print()

        # Step 1: Set up Visual Studio environment (Windows only)
        if not self.setup_visual_studio_env():
            print_error("Build process failed at Visual Studio setup")
            return False

        # Step 2: Check for .NET SDK
        self.check_dotnet_sdk()

        # Step 3: Clean (optional)
        if clean:
            self.clean_build()

        # Step 4: Configure CMake
        if not self.configure_cmake():
            print_error("Build process failed at CMake configuration")
            return False

        # Step 5: Build project
        if not self.build_project():
            print_error("Build process failed at compilation")
            return False

        # Step 6: Collect files
        if not self.collect_files():
            print_error("Build process failed at file collection")
            return False

        # Step 7: Create ZIP
        zip_path = self.create_zip()

        # Success!
        print_header("Build Completed Successfully!")
        print(f"{Colors.GREEN}Distribution package created:{Colors.RESET}")
        print(f"  {Colors.BOLD}{zip_path}{Colors.RESET}")
        print()

        return True


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Gravix Engine Release Build and Distribution System"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean previous build artifacts before building"
    )

    args = parser.parse_args()

    builder = GravixBuilder()
    success = builder.run(clean=args.clean)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
