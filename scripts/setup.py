#!/usr/bin/env python3
"""
Project Setup Script
--------------------
This script will:
1. Check and download Vulkan SDK if needed (version 1.4.321.1)
2. Verify GPU supports Vulkan 1.4
3. Initialize and update all git submodules recursively
4. Prepare the development environment

NOTE: This script does NOT run CMake. Use startup scripts for building.
"""

import os
import sys
import subprocess
import platform
from pathlib import Path

# Import the VulkanSDKManager
try:
    from vulkan_sdk_manager import VulkanSDKManager
except ImportError:
    print("ERROR: vulkan_sdk_manager.py not found!")
    print("Please ensure vulkan_sdk_manager.py is in the same directory as this script.")
    sys.exit(1)

# Import GPU capability checker
try:
    from gpu_capabilities import GPUCapabilityChecker
except ImportError:
    print("ERROR: gpu_capabilities.py not found!")
    print("Please ensure gpu_capabilities.py is in the same directory as this script.")
    sys.exit(1)


def print_section(title):
    """Print a formatted section header."""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70)


def print_error_box(title, messages):
    """Print an error message in a prominent box."""
    print("\n" + "!" * 70)
    print(f"  ERROR: {title}")
    print("!" * 70)
    for msg in messages:
        print(f"  {msg}")
    print("!" * 70)


def print_warning_box(title, messages):
    """Print a warning message in a prominent box."""
    print("\n" + "⚠" * 70)
    print(f"  WARNING: {title}")
    print("⚠" * 70)
    for msg in messages:
        print(f"  {msg}")
    print("⚠" * 70)


def check_git_available():
    """Check if git is available in the system."""
    try:
        result = subprocess.run(
            ["git", "--version"],
            capture_output=True,
            text=True,
            check=True
        )
        print(f"✓ Git found: {result.stdout.strip()}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("✗ Git not found! Please install Git and try again.")
        return False


def check_git_repository():
    """Check if we're in a git repository."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--git-dir"],
            capture_output=True,
            text=True,
            check=True
        )
        return True
    except subprocess.CalledProcessError:
        return False


def validate_vulkan_version(manager):
    """
    Validate that Vulkan 1.4 is installed with specific version check.
    Returns: (is_valid, requires_action, error_message)
    """
    is_installed, sdk_path, version = manager.check_installation()
    
    # No Vulkan SDK found at all
    if not is_installed:
        error_msgs = [
            f"Vulkan SDK {manager.REQUIRED_VERSION} is REQUIRED for this project.",
            "",
            "This project requires Vulkan 1.4 API features.",
            "An older version or no installation was detected.",
            "",
            "The setup script will download the required SDK for you."
        ]
        return False, True, error_msgs
    
    # Check if version starts with 1.4 (Vulkan 1.4.x.x)
    if version and not version.startswith("1.4"):
        error_msgs = [
            f"INCOMPATIBLE Vulkan SDK version detected!",
            "",
            f"  Found:    Vulkan SDK {version} at {sdk_path}",
            f"  Required: Vulkan SDK {manager.REQUIRED_VERSION} (Vulkan 1.4.x)",
            "",
            "This project requires Vulkan 1.4 API features which are not",
            "available in your currently installed version.",
            "",
            "The setup script will download the correct SDK version for you.",
        ]
        return False, True, error_msgs
    
    # Check if exact version matches
    if version != manager.REQUIRED_VERSION:
        warning_msgs = [
            f"Different Vulkan 1.4 SDK version detected.",
            "",
            f"  Found:    Vulkan SDK {version}",
            f"  Expected: Vulkan SDK {manager.REQUIRED_VERSION}",
            "",
            "Both versions support Vulkan 1.4 API, but for best compatibility",
            "we recommend using the exact version specified.",
            "",
            "The setup script will download the recommended SDK version."
        ]
        return False, True, warning_msgs
    
    # Correct version installed
    return True, False, None


def check_gpu_capabilities():
    """Check if system GPU supports Vulkan 1.4."""
    print_section("GPU Vulkan 1.4 Capability Check")
    
    checker = GPUCapabilityChecker()
    has_support, gpus, error_msg = checker.check_vulkan_support()
    
    # Print detailed GPU report
    checker.print_gpu_report(has_support, gpus, error_msg)
    
    # Analyze results
    if has_support:
        print("\n  ✓ Your GPU supports Vulkan 1.4!")
        return True, False, None
    
    # Check if we found GPUs but they don't support Vulkan 1.4
    if gpus:
        unsupported_gpus = [gpu for gpu in gpus if not gpu.get("supports_1_4", False)]
        unknown_support = any("Unknown" in gpu.get("api_version", "") for gpu in gpus)
        
        if unsupported_gpus and not unknown_support:
            # Definitely unsupported
            error_msgs = [
                "Your graphics card does NOT support Vulkan 1.4!",
                "",
                "Gravix Engine REQUIRES a GPU with Vulkan 1.4 support.",
                "Detected GPU(s):"
            ]
            for gpu in unsupported_gpus:
                error_msgs.append(f"  - {gpu['name']} (Max: {gpu['api_version']})")
            error_msgs.extend([
                "",
                "To run Gravix Engine, you will need to:",
                "  1. Update your GPU drivers to the latest version, OR",
                "  2. Upgrade to a GPU that supports Vulkan 1.4",
                "",
                "Minimum GPU requirements:",
                "  - NVIDIA: GTX 1000 series or newer",
                "  - AMD: RX 5000 series or newer",
                "  - Intel: Arc series or Iris Xe",
            ])
            return False, True, error_msgs
        else:
            # Unknown support - needs Vulkan SDK for verification
            warning_msgs = [
                "Cannot verify GPU Vulkan 1.4 support.",
                "",
                "Vulkan SDK is needed to accurately check your GPU capabilities.",
                "Detected GPU(s):"
            ]
            for gpu in gpus:
                warning_msgs.append(f"  - {gpu['name']}")
            warning_msgs.extend([
                "",
                "Please install the Vulkan SDK, then run this script again",
                "to verify your GPU supports Vulkan 1.4.",
            ])
            return False, False, warning_msgs
    
    # No GPUs detected at all
    warning_msgs = [
        "Could not detect any graphics cards.",
        "",
        "This could mean:",
        "  - No dedicated GPU is installed",
        "  - GPU drivers are not installed",
        "  - Detection tools are not available",
        "",
        "Gravix Engine REQUIRES a GPU with Vulkan 1.4 support.",
        "Please ensure you have:",
        "  1. A Vulkan 1.4 compatible GPU installed",
        "  2. Latest GPU drivers installed",
    ]
    return False, False, warning_msgs


def update_git_submodules():
    """Initialize and update all git submodules recursively."""
    print_section("Git Submodules Setup")
    
    # Go back one directory if we're in Scripts/
    original_dir = os.getcwd()
    
    # Check if we're in a Scripts directory and need to go up
    if os.path.basename(original_dir).lower() == 'scripts':
        parent_dir = os.path.dirname(original_dir)
        print(f"Current directory: {original_dir}")
        print(f"Changing to parent directory: {parent_dir}")
    else:
        parent_dir = original_dir
        print(f"Already in project root: {original_dir}")
    
    try:
        os.chdir(parent_dir)
        
        # Check if we're in a git repository
        if not check_git_repository():
            print("✗ Not in a git repository!")
            print(f"  Current location: {os.getcwd()}")
            print("  Please run this script from within your git repository.")
            os.chdir(original_dir)
            return False
        
        print(f"✓ Git repository detected")
        
        # Check for .gitmodules file
        gitmodules_path = Path(".gitmodules")
        if not gitmodules_path.exists():
            print("⚠ No .gitmodules file found.")
            print("  This repository may not have any submodules, or they haven't been configured.")
            os.chdir(original_dir)
            return True
        
        print(f"✓ Found .gitmodules file")
        
        # Read and display submodules
        print("\nSubmodules found:")
        try:
            with open(".gitmodules", "r") as f:
                content = f.read()
                # Simple parsing to show submodule paths
                for line in content.split("\n"):
                    if "path =" in line:
                        path = line.split("=")[1].strip()
                        print(f"  - {path}")
        except Exception as e:
            print(f"  (Could not read .gitmodules: {e})")
        
        # Initialize and update submodules
        print("\nInitializing and updating submodules...")
        print("This may take a few minutes depending on the size of the submodules...")
        
        cmd = ["git", "submodule", "update", "--init", "--recursive"]
        print(f"Running: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(
                cmd,
                check=True,
                text=True,
                capture_output=False  # Show output in real-time
            )
            print("\n✓ All submodules initialized and updated successfully!")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"\n✗ Failed to update submodules: {e}")
            return False
            
    finally:
        # Always return to original directory
        os.chdir(original_dir)


def setup_vulkan_sdk():
    """Setup Vulkan SDK with comprehensive version validation."""
    print_section("Vulkan SDK Setup")
    
    manager = VulkanSDKManager()
    
    # Validate Vulkan version
    is_valid, requires_action, messages = validate_vulkan_version(manager)
    
    if is_valid:
        _, sdk_path, version = manager.check_installation()
        print(f"✓ Vulkan SDK {version} is correctly installed!")
        print(f"  Location: {sdk_path}")
        print(f"  Vulkan 1.4 API features are available.")
        return True
    
    # Show error or warning based on severity
    is_installed, sdk_path, version = manager.check_installation()
    
    if not is_installed:
        print_error_box("Vulkan SDK Not Found", messages)
    elif version and not version.startswith("1.4"):
        print_error_box("Incompatible Vulkan Version", messages)
    else:
        print_warning_box("Vulkan SDK Version Mismatch", messages)
    
    # Download SDK
    print(f"\n{'=' * 70}")
    print(f"  Proceeding with Vulkan SDK {manager.REQUIRED_VERSION} download...")
    print(f"{'=' * 70}")
    
    try:
        installer_path = manager.download_sdk()
        print(f"\n✓ Vulkan SDK installer downloaded successfully!")
        print(f"  Location: {installer_path}")
        
        # Provide installation instructions
        system = platform.system().lower()
        
        print("\n" + "=" * 70)
        print("  NEXT STEPS - PLEASE READ CAREFULLY")
        print("=" * 70)
        
        if system == "windows":
            print(f"\n1. Run the installer:")
            print(f"   {installer_path}")
            print(f"\n2. Follow the installation wizard")
            print(f"\n3. **IMPORTANT**: After installation completes:")
            print(f"   - Close this terminal/command prompt")
            print(f"   - Open a NEW terminal/command prompt")
            print(f"   - Navigate back to this directory")
            print(f"   - Run this script again: python {os.path.basename(__file__)}")
            print(f"\n   This ensures the VULKAN_SDK environment variable is set correctly.")
            
        elif system == "linux":
            print(f"\n1. Extract the SDK:")
            print(f"   tar xf {installer_path}")
            print(f"\n2. Set up environment variables:")
            print(f"   export VULKAN_SDK=$PWD/{manager.REQUIRED_VERSION}/x86_64")
            print(f"   export PATH=$VULKAN_SDK/bin:$PATH")
            print(f"   export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH")
            print(f"   export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d")
            print(f"\n3. Add these to your ~/.bashrc or ~/.zshrc for persistence")
            print(f"\n4. **IMPORTANT**: After setting environment variables:")
            print(f"   - Source your shell config or open a new terminal")
            print(f"   - Run this script again: python3 {os.path.basename(__file__)}")
            
        elif system == "darwin":
            print(f"\n1. Open the DMG installer:")
            print(f"   open {installer_path}")
            print(f"\n2. Follow the installation instructions")
            print(f"\n3. **IMPORTANT**: After installation completes:")
            print(f"   - Close this terminal")
            print(f"   - Open a NEW terminal")
            print(f"   - Navigate back to this directory")
            print(f"   - Run this script again: python3 {os.path.basename(__file__)}")
        
        print("\n" + "=" * 70)
        return False  # Return False to indicate manual installation needed
        
    except Exception as e:
        print(f"\n✗ Failed to download Vulkan SDK: {e}")
        return False


def main():
    """Main setup routine."""
    print("=" * 70)
    print("  GRAVIX ENGINE - PROJECT SETUP")
    print("=" * 70)
    print(f"Platform: {platform.system()} {platform.machine()}")
    print(f"Python: {sys.version.split()[0]}")
    print("\nThis script will prepare your development environment.")
    print("It will NOT run CMake - use startup scripts for building.")
    
    # Step 1: Check Git
    print_section("Checking Prerequisites")
    if not check_git_available():
        print_error_box("Git Not Found", [
            "Git is required to clone submodules and manage the project.",
            "Please install Git and run this script again.",
            "",
            "Download Git from: https://git-scm.com/downloads"
        ])
        sys.exit(1)
    
    # Step 2: Check GPU capabilities (early warning)
    gpu_ok, gpu_critical, gpu_messages = check_gpu_capabilities()
    
    if gpu_critical:
        # Critical GPU issue - cannot continue
        print_error_box("GPU Does Not Support Vulkan 1.4", gpu_messages)
        print("\n" + "!" * 70)
        print("  SETUP CANNOT CONTINUE")
        print("!" * 70)
        print("\nYour hardware does not meet the minimum requirements.")
        print("Please upgrade your GPU or drivers before proceeding.")
        print("!" * 70)
        sys.exit(1)
    elif not gpu_ok and gpu_messages:
        # Warning - need SDK to verify
        print_warning_box("GPU Capabilities Unknown", gpu_messages)
        print("\nProceeding with Vulkan SDK installation for verification...")
    
    # Step 3: Setup Vulkan SDK with comprehensive validation
    vulkan_ready = setup_vulkan_sdk()
    
    if not vulkan_ready:
        print("\n" + "!" * 70)
        print("  SETUP PAUSED")
        print("!" * 70)
        print("\nPlease complete the Vulkan SDK installation as described above,")
        print("then run this script again to:")
        print("  1. Verify your GPU supports Vulkan 1.4")
        print("  2. Continue with git submodule setup")
        print("\nVulkan 1.4 is REQUIRED for Gravix Engine to compile and run.")
        print("!" * 70)
        sys.exit(0)
    
    # Step 4: Re-check GPU capabilities with Vulkan SDK installed
    if not gpu_ok:
        print("\n" + "=" * 70)
        print("  Re-checking GPU capabilities with Vulkan SDK...")
        print("=" * 70)
        
        gpu_ok_final, gpu_critical_final, gpu_messages_final = check_gpu_capabilities()
        
        if gpu_critical_final:
            print_error_box("GPU Does Not Support Vulkan 1.4", gpu_messages_final)
            print("\n" + "!" * 70)
            print("  CRITICAL: Hardware Incompatibility Detected")
            print("!" * 70)
            print("\nYour GPU does not support Vulkan 1.4.")
            print("Gravix Engine will not run on this system.")
            print("!" * 70)
            sys.exit(1)
        elif not gpu_ok_final:
            print_warning_box("GPU Verification Incomplete", gpu_messages_final or ["Could not fully verify GPU capabilities."])
            print("\nProceeding with caution...")
    
    # Step 5: Update Git Submodules
    submodules_updated = update_git_submodules()
    
    # Final status
    print_section("Setup Complete!")
    
    if vulkan_ready and submodules_updated and gpu_ok:
        print("\n✓ All setup tasks completed successfully!")
        print("\nYour development environment is ready:")
        print(f"  ✓ Vulkan SDK 1.4 ({VulkanSDKManager.REQUIRED_VERSION})")
        print(f"  ✓ GPU supports Vulkan 1.4")
        print(f"  ✓ Git submodules initialized")
        print("\n" + "=" * 70)
        print("  NEXT STEPS")
        print("=" * 70)
        print("\nTo configure and build the project, use the startup scripts:")
        print("  Windows:    startup.bat  or  .\\startup.ps1")
        print("  Linux/Mac:  ./startup.sh")
        print("\nThe startup scripts will run CMake and build the project.")
    else:
        print("\n⚠ Setup completed with some warnings:")
        if not vulkan_ready:
            print("  - Vulkan SDK needs attention")
        if not gpu_ok:
            print("  - GPU capabilities could not be fully verified")
        if not submodules_updated:
            print("  - Git submodules may need manual initialization")
        print("\nPlease review the messages above.")
        print("The engine may not work correctly if requirements are not met.")
    
    print("\n" + "=" * 70)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nSetup interrupted by user.")
        sys.exit(1)
    except Exception as e:
        print(f"\n\nUnexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)