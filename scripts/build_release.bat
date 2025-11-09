@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Gravix Engine - Release Build & Package Script
:: ============================================================================

echo.
echo ========================================
echo    Gravix Engine Release Builder
echo ========================================
echo.

:: ============================================================================
:: Configuration
:: ============================================================================

set /p CLEAN_BUILD="Clean previous build? (y/N): "
echo.

:: ============================================================================
:: Build
:: ============================================================================

echo Starting release build...
echo.

cd /d "%~dp0.."

if /i "%CLEAN_BUILD%"=="y" (
    python Scripts\build_release.py --clean
) else (
    python Scripts\build_release.py
)

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
echo Release package created successfully!
echo ========================================
echo   Location: dist\
echo ========================================
echo.

pause
