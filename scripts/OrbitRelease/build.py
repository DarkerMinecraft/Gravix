"""
Orbit Release Builder

Main orchestrator for building Orbit in release mode with all optimizations.
Automatically collects DLLs and creates a release package.
"""

import sys
import argparse
from pathlib import Path
import shutil
from datetime import datetime
import zipfile

from config import BuildConfig, DLLConfig, CompilerConfig
from cmake_builder import CMakeBuilder
from dll_collector import DLLCollector


class OrbitReleaseBuilder:
    """Main builder class that orchestrates the entire release build process"""

    def __init__(self, build_config: BuildConfig = None):
        self.build_config = build_config or BuildConfig.create_default()
        self.dll_config = DLLConfig()
        self.compiler_config = CompilerConfig()

        self.cmake_builder = CMakeBuilder(self.build_config, self.compiler_config)
        self.dll_collector = DLLCollector(self.build_config, self.dll_config)

    def build(self, skip_configure: bool = False, skip_build: bool = False) -> bool:
        """
        Execute the full build process

        Args:
            skip_configure: Skip CMake configuration step
            skip_build: Skip build step (useful for packaging only)

        Returns:
            True if successful, False otherwise
        """
        self._print_header()

        try:
            # Step 1: Configure
            if not skip_configure:
                if not self.cmake_builder.configure():
                    print("\n[ERROR] Configuration failed")
                    return False
            else:
                print("\n[INFO] Skipping configuration (--skip-configure)")

            # Step 2: Build
            if not skip_build:
                if not self.cmake_builder.build():
                    print("\n[ERROR] Build failed")
                    return False
            else:
                print("\n[INFO] Skipping build (--skip-build)")

            # Step 3: Collect DLLs
            if not self._collect_dependencies():
                print("\n[ERROR] DLL collection failed")
                return False

            # Step 4: Package release
            if not self._package_release():
                print("\n[ERROR] Packaging failed")
                return False

            self._print_success()
            return True

        except Exception as e:
            print(f"\n[ERROR] Build failed with exception: {e}")
            import traceback
            traceback.print_exc()
            return False

    def _collect_dependencies(self) -> bool:
        """Collect required DLLs"""
        print("\n" + "=" * 80)
        print("Collecting Dependencies")
        print("=" * 80)

        # Find the Orbit executable
        orbit_exe = self.build_config.build_dir / "bin" / "Orbit.exe"

        if not orbit_exe.exists():
            print(f"[ERROR] Orbit.exe not found at: {orbit_exe}")
            return False

        # Collect DLLs
        self.dll_collector.collect_dlls(orbit_exe)

        print(f"\n[DLL Collector] Found {len(self.dll_collector.collected_dlls)} required DLLs")

        return True

    def _package_release(self) -> bool:
        """Package the release build"""
        print("\n" + "=" * 80)
        print("Packaging Release")
        print("=" * 80)

        # Read version from VERSION file
        version = self._read_version()
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        package_name = f"Orbit_v{version}_{timestamp}"
        package_dir = self.build_config.output_dir / package_name

        print(f"\n[Packager] Creating package: {package_name}")
        print(f"[Packager] Version: {version}")

        # Clean and create package directory
        if package_dir.exists():
            shutil.rmtree(package_dir)
        package_dir.mkdir(parents=True, exist_ok=True)

        # Copy executables
        bin_dir = self.build_config.build_dir / "bin"
        if not self._copy_executables(bin_dir, package_dir):
            return False

        # Copy DLLs
        self.dll_collector.copy_dlls(package_dir)

        # Copy assets
        self._copy_assets(package_dir)

        # Copy Gravix-ScriptCore DLL
        self._copy_script_core(package_dir)

        # Copy ImGui configuration
        self._copy_imgui_config(package_dir)

        # Generate manifest
        manifest_path = package_dir / "dll_manifest.txt"
        self.dll_collector.generate_dll_manifest(manifest_path)

        # Create ZIP archive
        zip_path = self.build_config.output_dir / f"{package_name}.zip"
        self._create_zip(package_dir, zip_path)

        print(f"\n[Packager] Package created successfully:")
        print(f"  Directory: {package_dir}")
        print(f"  Archive:   {zip_path}")

        return True

    def _copy_executables(self, bin_dir: Path, package_dir: Path) -> bool:
        """Copy executable files to package directory"""
        print(f"\n[Packager] Copying executables from: {bin_dir}")

        executables = ["Orbit.exe"]

        for exe_name in executables:
            src = bin_dir / exe_name
            dst = package_dir / exe_name

            if not src.exists():
                print(f"  [ERROR] Executable not found: {src}")
                return False

            shutil.copy2(src, dst)
            print(f"  Copied: {exe_name}")

        return True

    def _copy_assets(self, package_dir: Path) -> None:
        """Copy asset directories to package"""
        print(f"\n[Packager] Copying assets")

        assets_dir = self.build_config.project_root / "Assets"
        if assets_dir.exists():
            dst_assets = package_dir / "Assets"
            shutil.copytree(assets_dir, dst_assets)
            print(f"  Copied: Assets/")

    def _copy_script_core(self, package_dir: Path) -> None:
        """Copy Gravix-ScriptCore DLL"""
        print(f"\n[Packager] Copying Gravix-ScriptCore")

        # Find Gravix-ScriptCore.dll in build directory
        script_core_dll = self.build_config.build_dir / "Gravix-ScriptCore" / "Gravix-ScriptCore.dll"

        if script_core_dll.exists():
            dst = package_dir / "Gravix-ScriptCore.dll"
            shutil.copy2(script_core_dll, dst)
            print(f"  Copied: Gravix-ScriptCore.dll")
        else:
            print(f"  [WARNING] Gravix-ScriptCore.dll not found at: {script_core_dll}")

    def _copy_imgui_config(self, package_dir: Path) -> None:
        """Copy ImGui configuration file"""
        print(f"\n[Packager] Copying ImGui configuration")

        # ImGui config is in the OrbitRelease directory
        script_dir = Path(__file__).parent
        imgui_ini = script_dir / "imgui.ini"

        if imgui_ini.exists():
            dst = package_dir / "imgui.ini"
            shutil.copy2(imgui_ini, dst)
            print(f"  Copied: imgui.ini")
        else:
            print(f"  [WARNING] imgui.ini not found at: {imgui_ini}")

    def _create_zip(self, source_dir: Path, zip_path: Path) -> None:
        """Create a ZIP archive of the package"""
        print(f"\n[Packager] Creating ZIP archive")

        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for file_path in source_dir.rglob('*'):
                if file_path.is_file():
                    arcname = file_path.relative_to(source_dir.parent)
                    zipf.write(file_path, arcname)

        # Get file size
        size_mb = zip_path.stat().st_size / (1024 * 1024)
        print(f"  Archive size: {size_mb:.2f} MB")

    def _read_version(self) -> str:
        """Read version from VERSION file in project root"""
        version_file = self.build_config.project_root / "VERSION"

        if version_file.exists():
            with open(version_file, 'r') as f:
                version = f.read().strip()
                return version
        else:
            print("[WARNING] VERSION file not found, using default version")
            return "1.0.0"

    def _print_header(self) -> None:
        """Print build header"""
        print("\n" + "=" * 80)
        print(" " * 25 + "Orbit Release Builder")
        print("=" * 80)
        print(f"  Project Root:   {self.build_config.project_root}")
        print(f"  Build Dir:      {self.build_config.build_dir}")
        print(f"  Output Dir:     {self.build_config.output_dir}")
        print(f"  Build Type:     {self.build_config.build_type}")
        print(f"  Generator:      {self.build_config.generator}")
        print(f"  Targets:        {', '.join(self.build_config.targets)}")
        print("=" * 80)

    def _print_success(self) -> None:
        """Print success message"""
        print("\n" + "=" * 80)
        print(" " * 30 + "BUILD SUCCESSFUL")
        print("=" * 80)
        print(f"\nRelease package created in: {self.build_config.output_dir}")
        print("\n")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description="Build Orbit in release mode with optimizations")

    parser.add_argument(
        "--skip-configure",
        action="store_true",
        help="Skip CMake configuration step"
    )

    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip build step (useful for re-packaging)"
    )

    parser.add_argument(
        "--build-dir",
        type=str,
        help="Custom build directory (default: build/OrbitRelease)"
    )

    parser.add_argument(
        "--output-dir",
        type=str,
        help="Custom output directory (default: dist/)"
    )

    parser.add_argument(
        "--jobs",
        type=int,
        help="Number of parallel build jobs"
    )

    parser.add_argument(
        "--verbosity",
        type=int,
        choices=[0, 1, 2],
        default=1,
        help="Ninja verbosity level (0=quiet, 1=default, 2=verbose)"
    )

    args = parser.parse_args()

    # Create build configuration
    config = BuildConfig.create_default()

    # Apply custom settings
    if args.build_dir:
        config.build_dir = Path(args.build_dir).resolve()

    if args.output_dir:
        config.output_dir = Path(args.output_dir).resolve()

    if args.jobs:
        config.parallel_jobs = args.jobs

    config.ninja_verbosity = args.verbosity

    # Create and run builder
    builder = OrbitReleaseBuilder(config)
    success = builder.build(
        skip_configure=args.skip_configure,
        skip_build=args.skip_build
    )

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
