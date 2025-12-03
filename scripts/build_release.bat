@echo off
REM ============================================================================
REM Orbit Release Build Script
REM
REM Wrapper for the Python-based Orbit Release Builder
REM Builds Gravix, Gravix-ScriptCore, and Orbit with full optimizations
REM ============================================================================

setlocal enabledelayedexpansion

REM Get script directory
set "SCRIPT_DIR=%~dp0"
set "BUILDER_DIR=%SCRIPT_DIR%OrbitRelease"

echo.
echo ================================================================================
echo                          Orbit Release Builder
echo ================================================================================
echo.

REM ============================================================================
REM Step 1: Setup MSVC Environment
REM ============================================================================

echo [1/3] Setting up MSVC environment...

REM Try to find Visual Studio using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    REM Find latest Visual Studio installation
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
        set "VS_PATH=%%i"
    )

    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" (
            echo   Found Visual Studio at: !VS_PATH!
            call "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
            goto msvc_setup_done
        )
    )
)

REM Manual detection fallback
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    goto msvc_setup_done
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    goto msvc_setup_done
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    goto msvc_setup_done
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    goto msvc_setup_done
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
    goto msvc_setup_done
)

echo [WARNING] Could not auto-detect Visual Studio
echo Continuing anyway (environment may already be set up)...

:msvc_setup_done
echo   MSVC environment ready
echo.

REM ============================================================================
REM Step 2: Check Prerequisites
REM ============================================================================

echo [2/3] Checking prerequisites...

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH
    echo Please install Python 3.7+ and add it to your PATH
    pause
    exit /b 1
)

REM Check CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is not installed or not in PATH
    echo Please install CMake 3.20+ and add it to your PATH
    pause
    exit /b 1
)

REM Check Ninja
ninja --version >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Ninja is not installed or not in PATH
    echo Install via: winget install Ninja-build.Ninja
    echo The build will still attempt to continue...
)

echo   All prerequisites OK
echo.

REM ============================================================================
REM Step 3: Run Python Build System
REM ============================================================================

echo [3/3] Running Orbit Release Builder...
echo.

REM Debug: Show all arguments
echo [DEBUG] All arguments: %*
echo [DEBUG] First argument: "%~1"
echo.

REM Parse command line arguments
set "EXTRA_ARGS="
set "SHOW_HELP=0"

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--help" set "SHOW_HELP=1"
if /i "%~1"=="-h" set "SHOW_HELP=1"
if /i "%~1"=="/?" set "SHOW_HELP=1"
set "EXTRA_ARGS=%EXTRA_ARGS% %~1"
shift
goto parse_args

:end_parse

REM Debug: Show final state
echo [DEBUG] SHOW_HELP: [%SHOW_HELP%]
echo [DEBUG] EXTRA_ARGS: [%EXTRA_ARGS%]
echo.

REM Show help if requested
if %SHOW_HELP%==1 goto show_help
goto skip_help

:show_help
echo Usage: build_release.bat [OPTIONS]
echo.
echo Options:
echo   --skip-configure      Skip CMake configuration step
echo   --skip-build          Skip build step (useful for re-packaging)
echo   --build-dir DIR       Custom build directory
echo   --output-dir DIR      Custom output directory
echo   --jobs N              Number of parallel build jobs
echo   --verbosity N         Ninja verbosity level (0=quiet, 1=default, 2=verbose)
echo   --help, -h, /?        Show this help message
echo.
echo Examples:
echo   build_release.bat
echo   build_release.bat --skip-configure
echo   build_release.bat --jobs 16 --verbosity 2
echo.
pause
exit /b 0

:skip_help
echo [DEBUG] Proceeding to build...

REM Run the Python build script
cd /d "%BUILDER_DIR%"

REM Debug: Show the command being executed
echo Running: python build.py%EXTRA_ARGS%
echo.

python build.py%EXTRA_ARGS%

set "BUILD_RESULT=%ERRORLEVEL%"

REM Check result
if %BUILD_RESULT% equ 0 (
    echo.
    echo ================================================================================
    echo                          BUILD SUCCESSFUL
    echo ================================================================================
    echo.
    echo Release package created in: dist\
    echo.
) else (
    echo.
    echo ================================================================================
    echo                          BUILD FAILED
    echo ================================================================================
    echo.
    echo Build exited with error code: %BUILD_RESULT%
    echo.
)

pause
exit /b %BUILD_RESULT%
