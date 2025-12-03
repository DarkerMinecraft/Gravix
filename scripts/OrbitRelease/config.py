"""
Build Configuration for Orbit Release Builder

This module contains all configuration settings for the Orbit release build system.
"""

import os
from pathlib import Path
from typing import List, Dict
from dataclasses import dataclass


@dataclass
class BuildConfig:
    """Build configuration settings"""

    # Project root directory
    project_root: Path

    # Build directory
    build_dir: Path

    # Output directory for release package
    output_dir: Path

    # Build type
    build_type: str = "Release"

    # Generator
    generator: str = "Ninja"

    # Targets to build (in order)
    targets: List[str] = None

    # CMake options
    cmake_options: Dict[str, str] = None

    # Ninja verbosity (0=quiet, 1=default, 2=verbose)
    ninja_verbosity: int = 1

    # Number of parallel jobs
    parallel_jobs: int = None

    def __post_init__(self):
        """Initialize default values"""
        if self.targets is None:
            self.targets = ["Gravix-ScriptCore", "Gravix", "Orbit"]

        if self.cmake_options is None:
            self.cmake_options = {
                "CMAKE_BUILD_TYPE": self.build_type,
                "GRAVIX_USE_VULKAN": "ON",
                "GRAVIX_BUILD_EDITOR": "ON",
                "GRAVIX_BUILD_SCRIPTING": "ON",
            }

        if self.parallel_jobs is None:
            # Use all available cores
            self.parallel_jobs = os.cpu_count() or 4

        # Ensure paths are absolute
        self.project_root = Path(self.project_root).resolve()
        self.build_dir = Path(self.build_dir).resolve()
        self.output_dir = Path(self.output_dir).resolve()

    @classmethod
    def create_default(cls) -> 'BuildConfig':
        """Create default build configuration"""
        # Assume script is in Scripts/OrbitRelease/
        script_dir = Path(__file__).parent
        project_root = script_dir.parent.parent

        return cls(
            project_root=project_root,
            build_dir=project_root / "build" / "OrbitRelease",
            output_dir=project_root / "dist",
        )


@dataclass
class DLLConfig:
    """DLL collection configuration"""

    # Directories to search for DLLs (relative to project root)
    search_dirs: List[str] = None

    # DLL patterns to include
    include_patterns: List[str] = None

    # DLL names to exclude
    exclude_dlls: List[str] = None

    # System DLL directories to skip
    skip_system_dirs: List[str] = None

    def __post_init__(self):
        """Initialize default values"""
        if self.search_dirs is None:
            self.search_dirs = [
                "ThirdParties/mono/bin/Release",
                "build/OrbitRelease/bin",
            ]

        if self.include_patterns is None:
            self.include_patterns = [
                "mono-2.0-sgen.dll",
            ]

        if self.exclude_dlls is None:
            # Windows system DLLs to exclude
            self.exclude_dlls = [
                "kernel32.dll",
                "user32.dll",
                "gdi32.dll",
                "winspool.drv",
                "comdlg32.dll",
                "advapi32.dll",
                "shell32.dll",
                "ole32.dll",
                "oleaut32.dll",
                "uuid.dll",
                "odbc32.dll",
                "odbccp32.dll",
                "vcruntime140.dll",
                "vcruntime140_1.dll",
                "msvcp140.dll",
                "ucrtbase.dll",
            ]

        if self.skip_system_dirs is None:
            self.skip_system_dirs = [
                r"C:\Windows",
                r"C:\Program Files",
                r"C:\Program Files (x86)",
            ]


@dataclass
class CompilerConfig:
    """Compiler optimization configuration"""

    # MSVC optimization flags
    msvc_flags: List[str] = None

    # Clang optimization flags
    clang_flags: List[str] = None

    # GCC optimization flags
    gcc_flags: List[str] = None

    def __post_init__(self):
        """Initialize default values"""
        if self.msvc_flags is None:
            self.msvc_flags = [
                "/O2",          # Maximum optimization
                "/Ob2",         # Inline expansion
                "/Oi",          # Enable intrinsic functions
                "/Ot",          # Favor fast code
                "/GL",          # Whole program optimization
                "/GF",          # String pooling
                "/Gy",          # Function-level linking
                "/fp:fast",     # Fast floating point
            ]

        if self.clang_flags is None:
            self.clang_flags = [
                "-O3",
                "-march=native",
                "-flto",
            ]

        if self.gcc_flags is None:
            self.gcc_flags = [
                "-O3",
                "-march=native",
                "-flto",
            ]
