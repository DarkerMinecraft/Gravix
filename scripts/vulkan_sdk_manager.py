import os
import sys
import platform
import subprocess
import urllib.request
import hashlib
from pathlib import Path
from typing import Optional, Tuple


class VulkanSDKManager:
    """
    Manages Vulkan SDK installation and verification.
    Automatically downloads and optionally installs Vulkan SDK if not found.
    """
    
    REQUIRED_VERSION = "1.4.321.1"
    BASE_DOWNLOAD_URL = "https://sdk.lunarg.com/sdk/download"
    
    def __init__(self, install_path: Optional[str] = None):
        """
        Initialize the Vulkan SDK Manager.
        
        Args:
            install_path: Custom installation path. If None, uses default location.
        """
        self.required_version = self.REQUIRED_VERSION
        self.install_path = install_path
        self.platform_info = self._detect_platform()
        
    def _detect_platform(self) -> dict:
        """Detect the current platform and architecture."""
        system = platform.system().lower()
        machine = platform.machine().lower()
        
        if system == "windows":
            arch = "X64" if machine in ["amd64", "x86_64"] else "ARM64"
            ext = "exe"
            platform_name = "windows" if arch == "X64" else "warm"
        elif system == "linux":
            arch = "x86_64"
            ext = "tar.xz"
            platform_name = "linux"
        elif system == "darwin":
            arch = platform.machine()
            ext = "dmg"
            platform_name = "mac"
        else:
            raise RuntimeError(f"Unsupported platform: {system}")
            
        return {
            "system": system,
            "arch": arch,
            "ext": ext,
            "platform_name": platform_name
        }
    
    def _get_download_url(self) -> str:
        """Generate the download URL for the required SDK version."""
        platform_name = self.platform_info["platform_name"]
        system = self.platform_info["system"]
        arch = self.platform_info["arch"]
        ext = self.platform_info["ext"]
        
        if system == "windows":
            filename = f"vulkansdk-windows-{arch}-{self.required_version}.{ext}"
        elif system == "linux":
            filename = f"vulkansdk-linux-{arch}-{self.required_version}.{ext}"
        elif system == "darwin":
            filename = f"vulkansdk-macos-{self.required_version}.{ext}"
        
        url = f"{self.BASE_DOWNLOAD_URL}/{self.required_version}/{platform_name}/{filename}"
        return url
    
    def check_installation(self) -> Tuple[bool, Optional[str], Optional[str]]:
        """
        Check if Vulkan SDK is installed and get its version.
        
        Returns:
            Tuple of (is_installed, sdk_path, version)
        """
        # Check VULKAN_SDK environment variable
        sdk_path = os.environ.get("VULKAN_SDK")
        
        if not sdk_path or not os.path.exists(sdk_path):
            return False, None, None
        
        # Try to determine version from path
        version = self._extract_version_from_path(sdk_path)
        
        return True, sdk_path, version
    
    def _extract_version_from_path(self, sdk_path: str) -> Optional[str]:
        """Extract SDK version from installation path."""
        # SDK path typically contains version like C:\VulkanSDK\1.4.321.1
        path_parts = Path(sdk_path).parts
        for part in path_parts:
            if self._is_version_string(part):
                return part
        return None
    
    def _is_version_string(self, s: str) -> bool:
        """Check if string looks like a version number."""
        parts = s.split('.')
        return len(parts) >= 3 and all(p.isdigit() for p in parts)
    
    def is_correct_version(self) -> bool:
        """Check if the installed SDK version matches the required version."""
        is_installed, _, version = self.check_installation()
        return is_installed and version == self.required_version
    
    def download_sdk(self, download_dir: Optional[str] = None) -> str:
        """
        Download the Vulkan SDK installer.
        
        Args:
            download_dir: Directory to download to. Defaults to current directory.
            
        Returns:
            Path to downloaded installer file.
        """
        if download_dir is None:
            download_dir = os.getcwd()
        
        os.makedirs(download_dir, exist_ok=True)
        
        url = self._get_download_url()
        filename = os.path.basename(url)
        filepath = os.path.join(download_dir, filename)
        
        # Check if already downloaded
        if os.path.exists(filepath):
            print(f"Installer already exists at: {filepath}")
            return filepath
        
        print(f"Downloading Vulkan SDK {self.required_version} from:")
        print(f"  {url}")
        print(f"  to {filepath}")
        print("This may take several minutes...")
        
        try:
            # Download with progress
            def report_progress(block_num, block_size, total_size):
                downloaded = block_num * block_size
                percent = min(100, (downloaded / total_size) * 100)
                print(f"\rProgress: {percent:.1f}%", end='', flush=True)
            
            urllib.request.urlretrieve(url, filepath, reporthook=report_progress)
            print("\nDownload completed successfully!")
            
        except Exception as e:
            if os.path.exists(filepath):
                os.remove(filepath)
            raise RuntimeError(f"Failed to download Vulkan SDK: {e}")
        
        return filepath
    
    def install_sdk(self, installer_path: str, silent: bool = True) -> bool:
        """
        Install the Vulkan SDK (Windows only for now).
        
        Args:
            installer_path: Path to the installer file.
            silent: If True, perform silent installation.
            
        Returns:
            True if installation successful, False otherwise.
        """
        if self.platform_info["system"] != "windows":
            print(f"Automatic installation not supported on {self.platform_info['system']}")
            print(f"Please run the installer manually: {installer_path}")
            return False
        
        if not os.path.exists(installer_path):
            raise FileNotFoundError(f"Installer not found: {installer_path}")
        
        # Default install path for Windows
        if self.install_path is None:
            self.install_path = f"C:\\VulkanSDK\\{self.required_version}"
        
        print(f"Installing Vulkan SDK to: {self.install_path}")
        
        if silent:
            # Silent installation with default components
            cmd = [
                installer_path,
                "--accept-licenses",
                "--default-answer",
                "--confirm-command",
                "install"
            ]
        else:
            # Interactive installation
            cmd = [installer_path]
        
        try:
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print("Installation completed successfully!")
            print("Please restart your command prompt/terminal to pick up environment variables.")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"Installation failed: {e}")
            if e.stdout:
                print(f"Output: {e.stdout}")
            if e.stderr:
                print(f"Error: {e.stderr}")
            return False
    
    def setup(self, auto_install: bool = False) -> bool:
        """
        Main setup method to check, download, and optionally install SDK.
        
        Args:
            auto_install: If True, automatically install after download (Windows only).
            
        Returns:
            True if SDK is ready, False otherwise.
        """
        print(f"Checking for Vulkan SDK {self.required_version}...")
        
        # Check if correct version is already installed
        if self.is_correct_version():
            _, sdk_path, version = self.check_installation()
            print(f"✓ Vulkan SDK {version} found at: {sdk_path}")
            return True
        
        # Check if any version is installed
        is_installed, sdk_path, version = self.check_installation()
        if is_installed:
            print(f"✗ Found Vulkan SDK {version}, but require {self.required_version}")
        else:
            print(f"✗ Vulkan SDK not found")
        
        # Download the SDK
        print(f"\nDownloading Vulkan SDK {self.required_version}...")
        try:
            installer_path = self.download_sdk()
        except Exception as e:
            print(f"Download failed: {e}")
            return False
        
        # Install if requested
        if auto_install and self.platform_info["system"] == "windows":
            return self.install_sdk(installer_path, silent=True)
        else:
            print(f"\nInstaller downloaded to: {installer_path}")
            if self.platform_info["system"] == "windows":
                print("Run the installer to complete installation:")
                print(f"  {installer_path}")
            elif self.platform_info["system"] == "linux":
                print("Extract and install with:")
                print(f"  tar xf {installer_path}")
                print(f"  export VULKAN_SDK=$PWD/{self.required_version}/x86_64")
            elif self.platform_info["system"] == "darwin":
                print("Open the DMG to install:")
                print(f"  open {installer_path}")
            return True
