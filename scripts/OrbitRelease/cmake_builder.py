"""
CMake Builder Module

Handles CMake configuration and Ninja build execution with Visual Studio-like output.
"""

import os
import subprocess
import sys
from pathlib import Path
from typing import List, Dict
import re
import time

from config import BuildConfig, CompilerConfig


class CMakeBuilder:
    """Handles CMake configuration and build process"""

    def __init__(self, build_config: BuildConfig, compiler_config: CompilerConfig):
        self.build_config = build_config
        self.compiler_config = compiler_config
        self.build_start_time = None

    def configure(self) -> bool:
        """
        Configure CMake project

        Returns:
            True if configuration succeeded, False otherwise
        """
        print("\n" + "=" * 80)
        print("CMake Configuration")
        print("=" * 80)

        # Clean build directory if it exists
        if self.build_config.build_dir.exists():
            print(f"[CMake] Cleaning build directory: {self.build_config.build_dir}")
            import shutil
            shutil.rmtree(self.build_config.build_dir)

        # Create build directory
        self.build_config.build_dir.mkdir(parents=True, exist_ok=True)

        # Build CMake command
        cmake_cmd = self._build_cmake_command()

        print(f"\n[CMake] Configuration command:")
        print(f"  {' '.join(cmake_cmd)}\n")

        # Run CMake configuration
        try:
            result = subprocess.run(
                cmake_cmd,
                cwd=self.build_config.build_dir,
                check=True
            )

            print("\n[CMake] Configuration completed successfully")
            return True

        except subprocess.CalledProcessError as e:
            print(f"\n[CMake] Configuration failed with exit code {e.returncode}")
            return False

    def build(self) -> bool:
        """
        Build the project using Ninja

        Returns:
            True if build succeeded, False otherwise
        """
        print("\n" + "=" * 80)
        print("Building Orbit Release")
        print("=" * 80)

        self.build_start_time = time.time()

        # Build each target in order
        for target in self.build_config.targets:
            if not self._build_target(target):
                return False

        # Print build summary
        self._print_build_summary()

        return True

    def _build_cmake_command(self) -> List[str]:
        """Build the CMake configuration command"""
        cmd = [
            "cmake",
            "-S", str(self.build_config.project_root),
            "-B", str(self.build_config.build_dir),
            "-G", self.build_config.generator,
        ]

        # Add CMake options
        for key, value in self.build_config.cmake_options.items():
            cmd.append(f"-D{key}={value}")

        # Add compiler-specific flags
        if sys.platform == "win32":
            # MSVC flags
            cxx_flags = " ".join(self.compiler_config.msvc_flags)
            cmd.append(f"-DCMAKE_CXX_FLAGS_RELEASE={cxx_flags}")

            # Linker flags for whole program optimization
            cmd.append("-DCMAKE_EXE_LINKER_FLAGS_RELEASE=/LTCG")
            cmd.append("-DCMAKE_SHARED_LINKER_FLAGS_RELEASE=/LTCG")

        return cmd

    def _build_target(self, target: str) -> bool:
        """
        Build a specific target

        Args:
            target: Target name to build

        Returns:
            True if build succeeded, False otherwise
        """
        print(f"\n[Ninja] Building target: {target}")
        print("-" * 80)

        # Build Ninja command with proper verbosity
        ninja_cmd = self._build_ninja_command(target)

        try:
            # Run Ninja with real-time output processing
            process = subprocess.Popen(
                ninja_cmd,
                cwd=self.build_config.build_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True
            )

            # Process output line by line for Visual Studio-like display
            for line in process.stdout:
                self._process_ninja_output(line, target)

            process.wait()

            if process.returncode == 0:
                print(f"\n[Ninja] Successfully built {target}")
                return True
            else:
                print(f"\n[Ninja] Build failed for {target} with exit code {process.returncode}")
                return False

        except Exception as e:
            print(f"\n[Ninja] Build error: {e}")
            return False

    def _build_ninja_command(self, target: str) -> List[str]:
        """Build the Ninja command"""
        cmd = ["cmake", "--build", str(self.build_config.build_dir)]

        # Add target
        cmd.extend(["--target", target])

        # Add config (for multi-config generators)
        cmd.extend(["--config", self.build_config.build_type])

        # Add parallel jobs
        cmd.extend(["--parallel", str(self.build_config.parallel_jobs)])

        # Ninja verbosity - use both methods to ensure output is shown
        if self.build_config.ninja_verbosity >= 1:
            cmd.append("--verbose")
            # Also pass -v to ninja directly
            cmd.extend(["--", "-v"])

        return cmd

    def _process_ninja_output(self, line: str, target: str) -> None:
        """
        Process and format Ninja output for Visual Studio-like display

        Args:
            line: Output line from Ninja
            target: Current target being built
        """
        line = line.rstrip()

        if not line:
            return

        # Pattern matching for different output types
        patterns = {
            'building': re.compile(r'\[(\d+)/(\d+)\]\s+Building\s+(\w+)\s+object\s+(.+)'),
            'linking': re.compile(r'\[(\d+)/(\d+)\]\s+Linking\s+(\w+)\s+(.+)'),
            'generating': re.compile(r'\[(\d+)/(\d+)\]\s+Generating\s+(.+)'),
            'percentage': re.compile(r'^\[\s*(\d+)%\]\s+(.+)'),
            'msvc_compile': re.compile(r'\[(\d+)/(\d+)\]\s+.*?cl\.exe.*?(?:/TP|/TC)?\s+.*?([A-Za-z]:\\.*?\.(?:cpp|c|cc|cxx))\s'),
            'ninja_compile': re.compile(r'\[(\d+)/(\d+)\]\s+.*?\\([^\\]+\.(?:cpp|c|cc|cxx|h|hpp))'),
        }

        # Check for MSVC compilation (cl.exe with full path to source file)
        match = patterns['msvc_compile'].search(line)
        if match:
            current, total, source_path = match.groups()
            # Extract just the filename from the full path
            source_file = source_path.split('\\')[-1]
            progress_num = int(current)
            total_num = int(total)
            percent = int((progress_num / total_num) * 100)

            # Visual Studio-like output: single line with filename
            print(f"  [{percent:3d}%] ({current}/{total}) {source_file:<60}", flush=True)
            return

        # Check for Ninja compilation (shorter format)
        match = patterns['ninja_compile'].search(line)
        if match:
            current, total, source_file = match.groups()
            progress_num = int(current)
            total_num = int(total)
            percent = int((progress_num / total_num) * 100)

            # Visual Studio-like output: single line with filename
            print(f"  [{percent:3d}%] ({current}/{total}) {source_file:<60}", flush=True)
            return

        # Check for building source files (CMake's formatted output)
        match = patterns['building'].search(line)
        if match:
            current, total, lang, obj_file = match.groups()
            # Extract source filename from object path
            source_file = self._extract_source_from_obj(obj_file)
            progress_num = int(current)
            total_num = int(total)
            percent = int((progress_num / total_num) * 100)

            # Visual Studio-like output: single line with filename
            print(f"  [{percent:3d}%] ({current}/{total}) {source_file:<60}", flush=True)
            return

        # Check for linking
        match = patterns['linking'].search(line)
        if match:
            current, total, link_type, output = match.groups()
            progress_num = int(current)
            total_num = int(total)
            percent = int((progress_num / total_num) * 100)
            print(f"  [{percent:3d}%] ({current}/{total}) Linking: {output}", flush=True)
            return

        # Check for generating
        match = patterns['generating'].search(line)
        if match:
            current, total, output = match.groups()
            progress_num = int(current)
            total_num = int(total)
            percent = int((progress_num / total_num) * 100)
            print(f"  [{percent:3d}%] ({current}/{total}) Generating: {output}", flush=True)
            return

        # Check for percentage-based progress
        match = patterns['percentage'].search(line)
        if match:
            percent, action = match.groups()
            # Show percentage progress for other actions
            print(f"  [{percent:3s}%] {action}", flush=True)
            return

        # Show errors and warnings always
        if any(keyword in line.lower() for keyword in ['error', 'warning', 'failed']):
            print(f"  {line}", flush=True)
            return

        # Filter out compiler command lines (they're handled above)
        if 'cl.exe' in line or 'clang' in line or 'g++' in line or 'gcc' in line:
            # These are compiler commands, skip them (already extracted filename above)
            return

        # Default: show other output only if verbosity >= 2
        if self.build_config.ninja_verbosity >= 2:
            print(f"  {line}", flush=True)

    def _extract_source_from_obj(self, obj_path: str) -> str:
        """Extract source filename from object file path"""
        # Object file path format: Gravix/CMakeFiles/Gravix.dir/Source/Core/Application.cpp.obj
        # We want: Application.cpp

        path = Path(obj_path)
        stem = path.stem

        # Remove .obj/.o extension artifacts
        if '.cpp' in stem:
            return stem + '.cpp'
        elif '.c' in stem:
            return stem[stem.rfind('.c'):] + '.c'
        else:
            return path.name

    def _print_build_summary(self) -> None:
        """Print build summary statistics"""
        if self.build_start_time:
            elapsed = time.time() - self.build_start_time
            minutes = int(elapsed // 60)
            seconds = int(elapsed % 60)

            print("\n" + "=" * 80)
            print(f"Build Summary")
            print("=" * 80)
            print(f"  Build Type:     {self.build_config.build_type}")
            print(f"  Generator:      {self.build_config.generator}")
            print(f"  Parallel Jobs:  {self.build_config.parallel_jobs}")
            print(f"  Targets Built:  {', '.join(self.build_config.targets)}")
            print(f"  Build Time:     {minutes}m {seconds}s")
            print("=" * 80)
