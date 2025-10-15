#!/bin/bash
# Gravix Engine - Setup Wrapper (Linux/macOS)
# This script runs setup.py with the appropriate Python interpreter

echo "Running Gravix Engine Setup..."
echo ""

# Try python3 first, then python
if command -v python3 &> /dev/null; then
    python3 setup.py "$@"
elif command -v python &> /dev/null; then
    python setup.py "$@"
else
    echo "ERROR: Python not found!"
    echo ""
    echo "Please install Python 3.8 or newer:"
    echo "  Ubuntu/Debian: sudo apt-get install python3"
    echo "  macOS:         brew install python3"
    echo ""
    exit 1
fi