import os
import sys
import platform
import subprocess
import json
from typing import Optional, Dict, List, Tuple


class GPUCapabilityChecker:
    """
    Checks GPU capabilities and Vulkan API version support.
    """
    
    VULKAN_1_4_VERSION = 0x00403000  # Vulkan 1.4.0 in hex format
    
    def __init__(self):
        self.platform_info = self._detect_platform()
        self.gpus = []
        
    def _detect_platform(self) -> dict:
        """Detect the current platform."""
        system = platform.system().lower()
        return {"system": system}
    
    def check_vulkan_support(self) -> Tuple[bool, List[Dict], str]:
        """
        Check if system GPUs support Vulkan 1.4.
        
        Returns:
            Tuple of (has_vulkan_1_4_support, gpu_list, error_message)
        """
        # Try to use vulkaninfo if available
        vulkaninfo_result = self._check_with_vulkaninfo()
        if vulkaninfo_result is not None:
            return vulkaninfo_result
        
        # Fallback to system-specific GPU detection
        print("  ⚠ vulkaninfo not available, falling back to basic GPU detection...")
        return self._check_gpu_basic()
    
    def _check_with_vulkaninfo(self) -> Optional[Tuple[bool, List[Dict], str]]:
        """
        Use vulkaninfo to check GPU Vulkan capabilities.
        Returns None if vulkaninfo is not available.
        """
        try:
            # Try JSON output first (more reliable to parse)
            result = subprocess.run(
                ["vulkaninfo", "--json"],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                return self._parse_vulkaninfo_json(result.stdout)
            
            # Try regular vulkaninfo output
            result = subprocess.run(
                ["vulkaninfo", "--summary"],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                return self._parse_vulkaninfo_text(result.stdout)
                
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass
        
        return None
    
    def _parse_vulkaninfo_json(self, output: str) -> Tuple[bool, List[Dict], str]:
        """Parse vulkaninfo JSON output."""
        try:
            data = json.loads(output)
            gpus = []
            has_1_4_support = False
            
            # Check if there are any physical devices
            if "capabilities" not in data or "device" not in data["capabilities"]:
                return False, [], "No Vulkan-capable GPUs detected."
            
            devices = data["capabilities"]["device"]
            if not devices:
                return False, [], "No Vulkan-capable GPUs detected."
            
            for device in devices:
                props = device.get("properties", {})
                gpu_name = props.get("deviceName", "Unknown GPU")
                api_version = props.get("apiVersion", 0)
                
                # Parse Vulkan version
                major = (api_version >> 22) & 0x3FF
                minor = (api_version >> 12) & 0x3FF
                patch = api_version & 0xFFF
                version_str = f"{major}.{minor}.{patch}"
                
                supports_1_4 = api_version >= self.VULKAN_1_4_VERSION
                
                gpu_info = {
                    "name": gpu_name,
                    "api_version": version_str,
                    "supports_1_4": supports_1_4,
                    "vendor_id": props.get("vendorID", "Unknown")
                }
                
                gpus.append(gpu_info)
                if supports_1_4:
                    has_1_4_support = True
            
            if not has_1_4_support:
                return False, gpus, "No GPU supports Vulkan 1.4 or higher."
            
            return True, gpus, ""
            
        except (json.JSONDecodeError, KeyError, ValueError) as e:
            return False, [], f"Failed to parse vulkaninfo output: {e}"
    
    def _parse_vulkaninfo_text(self, output: str) -> Tuple[bool, List[Dict], str]:
        """Parse vulkaninfo text output."""
        gpus = []
        has_1_4_support = False
        current_gpu = None
        
        for line in output.split("\n"):
            line = line.strip()
            
            # Look for GPU device lines
            if "GPU" in line and ":" in line:
                if current_gpu:
                    gpus.append(current_gpu)
                current_gpu = {"name": "Unknown", "api_version": "Unknown", "supports_1_4": False}
            
            # Extract device name
            if current_gpu and "deviceName" in line and "=" in line:
                name = line.split("=", 1)[1].strip()
                current_gpu["name"] = name
            
            # Extract API version
            if current_gpu and "apiVersion" in line and "=" in line:
                version_part = line.split("=", 1)[1].strip()
                # Parse version like "1.4.280" or hex value
                if "." in version_part:
                    version_str = version_part.split()[0]
                    current_gpu["api_version"] = version_str
                    
                    parts = version_str.split(".")
                    if len(parts) >= 2:
                        major, minor = int(parts[0]), int(parts[1])
                        if major > 1 or (major == 1 and minor >= 4):
                            current_gpu["supports_1_4"] = True
                            has_1_4_support = True
        
        if current_gpu:
            gpus.append(current_gpu)
        
        if not gpus:
            return False, [], "No Vulkan-capable GPUs detected."
        
        if not has_1_4_support:
            return False, gpus, "No GPU supports Vulkan 1.4 or higher."
        
        return True, gpus, ""
    
    def _check_gpu_basic(self) -> Tuple[bool, List[Dict], str]:
        """
        Basic GPU detection without Vulkan runtime.
        This provides limited information but can detect GPU model.
        """
        system = self.platform_info["system"]
        
        if system == "windows":
            return self._check_gpu_windows()
        elif system == "linux":
            return self._check_gpu_linux()
        elif system == "darwin":
            return self._check_gpu_macos()
        else:
            return False, [], f"Unsupported platform: {system}"
    
    def _check_gpu_windows(self) -> Tuple[bool, List[Dict], str]:
        """Check GPU on Windows using WMIC."""
        try:
            result = subprocess.run(
                ["wmic", "path", "win32_VideoController", "get", "name"],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                lines = result.stdout.strip().split("\n")[1:]  # Skip header
                gpus = []
                
                for line in lines:
                    gpu_name = line.strip()
                    if gpu_name:
                        # Basic heuristic: newer GPUs likely support Vulkan 1.4
                        supports_1_4 = self._estimate_vulkan_support(gpu_name)
                        gpus.append({
                            "name": gpu_name,
                            "api_version": "Unknown (Vulkan SDK needed for detection)",
                            "supports_1_4": supports_1_4
                        })
                
                if gpus:
                    # Conservative: can't verify without vulkaninfo
                    return False, gpus, "Install Vulkan SDK to verify GPU capabilities."
                    
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        
        return False, [], "Could not detect GPU. Install Vulkan SDK for proper detection."
    
    def _check_gpu_linux(self) -> Tuple[bool, List[Dict], str]:
        """Check GPU on Linux using lspci."""
        try:
            result = subprocess.run(
                ["lspci"],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                gpus = []
                for line in result.stdout.split("\n"):
                    if "VGA" in line or "3D" in line or "Display" in line:
                        # Extract GPU name
                        parts = line.split(":", 2)
                        if len(parts) >= 3:
                            gpu_name = parts[2].strip()
                            supports_1_4 = self._estimate_vulkan_support(gpu_name)
                            gpus.append({
                                "name": gpu_name,
                                "api_version": "Unknown (Vulkan SDK needed for detection)",
                                "supports_1_4": supports_1_4
                            })
                
                if gpus:
                    return False, gpus, "Install Vulkan SDK to verify GPU capabilities."
                    
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        
        return False, [], "Could not detect GPU. Install Vulkan SDK for proper detection."
    
    def _check_gpu_macos(self) -> Tuple[bool, List[Dict], str]:
        """Check GPU on macOS using system_profiler."""
        try:
            result = subprocess.run(
                ["system_profiler", "SPDisplaysDataType"],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                gpus = []
                for line in result.stdout.split("\n"):
                    if "Chipset Model:" in line:
                        gpu_name = line.split(":", 1)[1].strip()
                        # macOS with MoltenVK support
                        supports_1_4 = "Apple" in gpu_name or "AMD" in gpu_name or "Intel" in gpu_name
                        gpus.append({
                            "name": gpu_name,
                            "api_version": "Via MoltenVK",
                            "supports_1_4": supports_1_4
                        })
                
                if gpus:
                    return False, gpus, "Install Vulkan SDK to verify GPU capabilities."
                    
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        
        return False, [], "Could not detect GPU. Install Vulkan SDK for proper detection."
    
    def _estimate_vulkan_support(self, gpu_name: str) -> bool:
        """
        Estimate if GPU likely supports Vulkan 1.4 based on name.
        This is a rough heuristic and should not be fully trusted.
        """
        gpu_lower = gpu_name.lower()
        
        # NVIDIA GPUs - GTX 10xx series and newer, RTX series
        if "nvidia" in gpu_lower or "geforce" in gpu_lower:
            if any(x in gpu_lower for x in ["rtx", "gtx 16", "gtx 20", "gtx 30", "gtx 40"]):
                return True
            # GTX 10xx series
            if "gtx 10" in gpu_lower:
                return True
        
        # AMD GPUs - RX 5000 series and newer
        if "amd" in gpu_lower or "radeon" in gpu_lower:
            if any(x in gpu_lower for x in ["rx 5", "rx 6", "rx 7", "vega"]):
                return True
        
        # Intel Arc and newer integrated
        if "intel" in gpu_lower:
            if any(x in gpu_lower for x in ["arc", "iris xe", "uhd 7"]):
                return True
        
        # Apple Silicon
        if "apple" in gpu_lower and any(x in gpu_lower for x in ["m1", "m2", "m3"]):
            return True
        
        # Unknown or older GPU - conservative estimate
        return False
    
    def print_gpu_report(self, has_support: bool, gpus: List[Dict], error_msg: str):
        """Print a formatted GPU capability report."""
        print("\n  Graphics Card(s) Detected:")
        print("  " + "-" * 66)
        
        if not gpus:
            print(f"  ⚠ {error_msg}")
            return
        
        for i, gpu in enumerate(gpus, 1):
            name = gpu.get("name", "Unknown")
            version = gpu.get("api_version", "Unknown")
            supports = gpu.get("supports_1_4", False)
            
            status = "✓" if supports else "✗"
            print(f"  {status} GPU {i}: {name}")
            print(f"     Vulkan API: {version}")
            
            if supports:
                print(f"     Status: Vulkan 1.4 supported ✓")
            else:
                print(f"     Status: Vulkan 1.4 support unknown or unsupported")
        
        if error_msg:
            print(f"\n  Note: {error_msg}")