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

        # Check if already configured
        cmake_cache = self.build_config.build_dir / "CMakeCache.txt"
        if cmake_cache.exists():
            print(f"[CMake] Build directory already configured: {self.build_config.build_dir}")
            print(f"[CMake] Skipping configuration (use --clean to reconfigure)")
            return True

        # Clean build directory if it exists but not configured
        if self.build_config.build_dir.exists():
            print(f"[CMake] Cleaning incomplete build directory: {self.build_config.build_dir}")
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

        # No custom compiler flags - use default Release settings (same as RelWithDebInfo)

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

        # Only add verbose flags if verbosity is 2 or higher (shows full compiler commands)
        if self.build_config.ninja_verbosity >= 2:
            cmd.append("--verbose")
            # Also pass -v to ninja directly
            cmd.extend(["--", "-v"])

        return cmd

    def _process_ninja_output(self, line: str, target: str) -> None:
        """
        Process and format Ninja output - shows clean Ninja progress format

        Args:
            line: Output line from Ninja
            target: Current target being built
        """
        line = line.rstrip()

        if not line:
            return

        # In verbose mode (level 2), show everything
        if self.build_config.ninja_verbosity >= 2:
            print(f"  {line}", flush=True)
            return

        # Default mode (level 0-1): Show clean Ninja progress
        # Pattern matching for Ninja progress output
        progress_pattern = re.compile(r'^\[(\d+)/(\d+)\]')

        # Show Ninja progress lines: [28/669] Building CXX object ...
        if progress_pattern.match(line):
            print(f"  {line}", flush=True)
            return

        # Always show errors and warnings
        if any(keyword in line.lower() for keyword in ['error', 'warning', 'failed']):
            print(f"  {line}", flush=True)
            return

        # Filter out everything else (CMake messages, etc.) in default mode
        # Only shown in verbosity level 2

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
