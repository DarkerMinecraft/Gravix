@echo off
REM Gravix Engine - Setup Wrapper (Windows)
REM This script runs setup.py with the appropriate Python interpreter

echo Running Gravix Engine Setup...
echo.

REM Try python3 first, then python
where python3 >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    python3 setup.py %*
) else (
    where python >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        python setup.py %*
    ) else (
        echo ERROR: Python not found!
        echo.
        echo Please install Python 3.8 or newer from:
        echo https://www.python.org/downloads/
        echo.
        pause
        exit /b 1
    )
)

pause