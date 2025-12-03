"""
Orbit Release Builder

Modular build system for creating optimized release builds of Orbit.
"""

__version__ = "1.0.0"
__author__ = "Gravix Engine Team"

from .config import BuildConfig, DLLConfig, CompilerConfig
from .cmake_builder import CMakeBuilder
from .dll_collector import DLLCollector
from .build import OrbitReleaseBuilder

__all__ = [
    "BuildConfig",
    "DLLConfig",
    "CompilerConfig",
    "CMakeBuilder",
    "DLLCollector",
    "OrbitReleaseBuilder",
]
