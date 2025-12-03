"""
DLL Collector Module

Automatically detects and collects required DLLs for the Orbit release build.
"""

import os
import shutil
from pathlib import Path
from typing import List, Set
import subprocess
import re

from config import BuildConfig, DLLConfig


class DLLCollector:
    """Collects and copies required DLLs for the release package"""

    def __init__(self, build_config: BuildConfig, dll_config: DLLConfig):
        self.build_config = build_config
        self.dll_config = dll_config
        self.collected_dlls: Set[Path] = set()

    def collect_dlls(self, executable_path: Path) -> List[Path]:
        """
        Collect all required DLLs for the given executable

        Args:
            executable_path: Path to the executable to analyze

        Returns:
            List of DLL paths that need to be copied
        """
        print(f"\n[DLL Collector] Analyzing dependencies for: {executable_path.name}")

        # Find DLLs using dependency walker or manual search
        required_dlls = self._find_dependencies(executable_path)

        # Filter out system DLLs
        filtered_dlls = self._filter_system_dlls(required_dlls)

        # Add to collected set
        self.collected_dlls.update(filtered_dlls)

        return filtered_dlls

    def _find_dependencies(self, executable_path: Path) -> Set[Path]:
        """Find DLL dependencies for an executable"""
        dependencies = set()

        # Method 1: Use dumpbin (if available)
        dependencies.update(self._find_with_dumpbin(executable_path))

        # Method 2: Search known directories for common DLLs
        dependencies.update(self._search_known_directories())

        return dependencies

    def _find_with_dumpbin(self, executable_path: Path) -> Set[Path]:
        """Use dumpbin to find dependencies"""
        dependencies = set()

        try:
            result = subprocess.run(
                ["dumpbin", "/dependents", str(executable_path)],
                capture_output=True,
                text=True,
                timeout=30
            )

            if result.returncode == 0:
                # Parse dumpbin output
                lines = result.stdout.split('\n')
                in_dependencies = False

                for line in lines:
                    line = line.strip()

                    if "Image has the following dependencies:" in line:
                        in_dependencies = True
                        continue

                    if in_dependencies and line.endswith('.dll'):
                        dll_name = line.lower()

                        # Skip system DLLs
                        if dll_name not in [d.lower() for d in self.dll_config.exclude_dlls]:
                            # Try to locate the DLL
                            dll_path = self._locate_dll(dll_name)
                            if dll_path:
                                dependencies.add(dll_path)

        except (subprocess.TimeoutExpired, FileNotFoundError):
            print("[DLL Collector] Warning: dumpbin not found, using manual search")

        return dependencies

    def _search_known_directories(self) -> Set[Path]:
        """Search known directories for required DLLs"""
        dependencies = set()

        for search_dir in self.dll_config.search_dirs:
            dir_path = self.build_config.project_root / search_dir

            if not dir_path.exists():
                continue

            print(f"[DLL Collector] Searching: {search_dir}")

            # Search for DLLs matching patterns
            for pattern in self.dll_config.include_patterns:
                # Convert glob pattern to actual files
                if '*' in pattern:
                    # Pattern with wildcard
                    for dll_file in dir_path.glob(pattern):
                        if dll_file.suffix.lower() == '.dll':
                            dependencies.add(dll_file)
                            print(f"  Found: {dll_file.name}")
                else:
                    # Exact filename
                    dll_file = dir_path / pattern
                    if dll_file.exists():
                        dependencies.add(dll_file)
                        print(f"  Found: {dll_file.name}")

        return dependencies

    def _locate_dll(self, dll_name: str) -> Path | None:
        """Locate a DLL by name in search directories"""
        for search_dir in self.dll_config.search_dirs:
            dir_path = self.build_config.project_root / search_dir
            dll_path = dir_path / dll_name

            if dll_path.exists():
                return dll_path

        return None

    def _filter_system_dlls(self, dlls: Set[Path]) -> Set[Path]:
        """Filter out Windows system DLLs"""
        filtered = set()

        for dll_path in dlls:
            # Check if in system directory
            is_system = False
            for system_dir in self.dll_config.skip_system_dirs:
                if str(dll_path).lower().startswith(system_dir.lower()):
                    is_system = True
                    break

            # Check if in exclude list
            if dll_path.name.lower() in [d.lower() for d in self.dll_config.exclude_dlls]:
                is_system = True

            if not is_system:
                filtered.add(dll_path)

        return filtered

    def copy_dlls(self, destination_dir: Path) -> None:
        """
        Copy all collected DLLs to the destination directory

        Args:
            destination_dir: Directory to copy DLLs to
        """
        destination_dir.mkdir(parents=True, exist_ok=True)

        print(f"\n[DLL Collector] Copying DLLs to: {destination_dir}")

        for dll_path in sorted(self.collected_dlls):
            dest_path = destination_dir / dll_path.name

            try:
                shutil.copy2(dll_path, dest_path)
                print(f"  Copied: {dll_path.name}")
            except Exception as e:
                print(f"  Error copying {dll_path.name}: {e}")

        print(f"[DLL Collector] Copied {len(self.collected_dlls)} DLLs")

    def generate_dll_manifest(self, output_path: Path) -> None:
        """
        Generate a manifest file listing all collected DLLs

        Args:
            output_path: Path to save the manifest file
        """
        with open(output_path, 'w') as f:
            f.write("# DLL Manifest\n")
            f.write("# Auto-generated by Orbit Release Builder\n\n")

            for dll_path in sorted(self.collected_dlls):
                f.write(f"{dll_path.name} <- {dll_path}\n")

        print(f"[DLL Collector] Generated manifest: {output_path}")
