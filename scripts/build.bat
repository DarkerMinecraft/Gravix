@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Gravix Engine - Interactive Build Script
:: ============================================================================

echo.
echo ========================================
echo    Gravix Engine Build Configuration
echo ========================================
echo.

:: ============================================================================
:: Build Configuration
:: ============================================================================

:build_type_menu
echo Select Build Type:
echo   [1] Debug
echo   [2] Release
echo   [3] RelWithDebInfo
echo.
set /p BUILD_TYPE_CHOICE="Your choice (1-3): "

if "%BUILD_TYPE_CHOICE%"=="1" (
    set BUILD_TYPE=Debug
) else if "%BUILD_TYPE_CHOICE%"=="2" (
    set BUILD_TYPE=Release
) else if "%BUILD_TYPE_CHOICE%"=="3" (
    set BUILD_TYPE=RelWithDebInfo
) else (
    echo Invalid choice! Please try again.
    echo.
    goto build_type_menu
)

echo   Selected: %BUILD_TYPE%
echo.

:: ============================================================================
:: Feature Configuration
:: ============================================================================

echo Configure Features:
echo.

:: Vulkan
set /p USE_VULKAN="Enable Vulkan renderer? (Y/n): "
if /i "%USE_VULKAN%"=="n" (
    set VULKAN_FLAG=-DGRAVIX_USE_VULKAN=OFF
    echo   Vulkan: Disabled
) else (
    set VULKAN_FLAG=-DGRAVIX_USE_VULKAN=ON
    echo   Vulkan: Enabled
)
echo.

:: Editor
set /p BUILD_EDITOR="Build Orbit editor? (Y/n): "
if /i "%BUILD_EDITOR%"=="n" (
    set EDITOR_FLAG=-DGRAVIX_BUILD_EDITOR=OFF
    echo   Editor: Disabled
) else (
    set EDITOR_FLAG=-DGRAVIX_BUILD_EDITOR=ON
    echo   Editor: Enabled
)
echo.

:: Scripting
set /p BUILD_SCRIPTING="Build C# scripting support? (Y/n): "
if /i "%BUILD_SCRIPTING%"=="n" (
    set SCRIPTING_FLAG=-DGRAVIX_BUILD_SCRIPTING=OFF
    echo   Scripting: Disabled
) else (
    set SCRIPTING_FLAG=-DGRAVIX_BUILD_SCRIPTING=ON
    echo   Scripting: Enabled
)
echo.

:: ============================================================================
:: Generator Selection
:: ============================================================================

:generator_menu
echo Select Generator:
echo   [1] Ninja (Fast, command-line)
echo   [2] Visual Studio 2022
echo   [3] Visual Studio 2019
echo.
set /p GENERATOR_CHOICE="Your choice (1-3): "

if "%GENERATOR_CHOICE%"=="1" (
    set GENERATOR=Ninja
    set GENERATOR_FLAG=-G "Ninja"
) else if "%GENERATOR_CHOICE%"=="2" (
    set GENERATOR=Visual Studio 2022
    set GENERATOR_FLAG=-G "Visual Studio 17 2022"
) else if "%GENERATOR_CHOICE%"=="3" (
    set GENERATOR=Visual Studio 2019
    set GENERATOR_FLAG=-G "Visual Studio 16 2019"
) else (
    echo Invalid choice! Please try again.
    echo.
    goto generator_menu
)

echo   Selected: %GENERATOR%
echo.

:: ============================================================================
:: Clean Build Option
:: ============================================================================

set /p CLEAN_BUILD="Clean previous build? (y/N): "
if /i "%CLEAN_BUILD%"=="y" (
    echo   Cleaning build directory...
    if exist "build\%BUILD_TYPE%" (
        rmdir /s /q "build\%BUILD_TYPE%"
    )
    echo   Done!
)
echo.

:: ============================================================================
:: Summary
:: ============================================================================

echo ========================================
echo Build Summary
echo ========================================
echo   Build Type:   %BUILD_TYPE%
echo   Generator:    %GENERATOR%
echo   Vulkan:       %VULKAN_FLAG:-DGRAVIX_USE_VULKAN=%
echo   Editor:       %EDITOR_FLAG:-DGRAVIX_BUILD_EDITOR=%
echo   Scripting:    %SCRIPTING_FLAG:-DGRAVIX_BUILD_SCRIPTING=%
echo ========================================
echo.

set /p CONFIRM="Proceed with build? (Y/n): "
if /i "%CONFIRM%"=="n" (
    echo Build cancelled.
    pause
    exit /b 0
)

:: ============================================================================
:: CMake Configuration
:: ============================================================================

echo.
echo [1/2] Configuring CMake...
echo ========================================

:: Set up Visual Studio environment if using Ninja
if "%GENERATOR%"=="Ninja" (
    echo Setting up Visual Studio environment...

    :: Try VS 2022 first
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Visual Studio not found!
        pause
        exit /b 1
    )
)

cmake -S . -B build\%BUILD_TYPE% ^
    %GENERATOR_FLAG% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    %VULKAN_FLAG% ^
    %EDITOR_FLAG% ^
    %SCRIPTING_FLAG%

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

:: ============================================================================
:: Build
:: ============================================================================

echo.
echo [2/2] Building project...
echo ========================================

cmake --build build\%BUILD_TYPE% --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

:: ============================================================================
:: Success
:: ============================================================================

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo   Output: build\%BUILD_TYPE%\bin
echo ========================================
echo.

pause
