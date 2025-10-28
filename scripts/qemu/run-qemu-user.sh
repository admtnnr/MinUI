#!/bin/sh
#
# run-qemu-user.sh - Run MinUI ARM binaries under qemu-user emulation
#
# This script runs minui.elf or minarch.elf from an existing build/SYSTEM/rg35xx
# using qemu-user mode (qemu-arm or qemu-arm-static). It's designed for quick
# development testing on macOS (M1/ARM64) and Linux hosts.
#
# Usage:
#   ./scripts/qemu/run-qemu-user.sh [minui|minarch]
#
# Prerequisites:
#   - macOS: brew install qemu
#   - Linux: apt install qemu-user-static (or equivalent)
#   - Build artifacts in build/SYSTEM/rg35xx (run: make PLATFORM=rg35xx build)
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$REPO_ROOT/build/SYSTEM/rg35xx"
BIN_DIR="$BUILD_DIR/bin"
LIB_DIR="$BUILD_DIR/lib"

# Default binary to run
BINARY="${1:-minui}"

# Detect qemu-arm binary
find_qemu_arm() {
    if command -v qemu-arm >/dev/null 2>&1; then
        echo "qemu-arm"
    elif command -v qemu-arm-static >/dev/null 2>&1; then
        echo "qemu-arm-static"
    else
        echo ""
    fi
}

# Main execution
main() {
    echo "==================================================="
    echo "MinUI QEMU User-Mode Emulation"
    echo "==================================================="
    echo ""
    
    # Check for qemu-arm
    QEMU_ARM="$(find_qemu_arm)"
    if [ -z "$QEMU_ARM" ]; then
        echo "ERROR: qemu-arm or qemu-arm-static not found in PATH"
        echo ""
        echo "Install QEMU:"
        echo "  macOS:  brew install qemu"
        echo "  Linux:  sudo apt install qemu-user-static"
        echo "          or equivalent for your distribution"
        echo ""
        exit 1
    fi
    
    echo "Found QEMU: $QEMU_ARM"
    echo ""
    
    # Verify build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        echo "ERROR: Build directory not found: $BUILD_DIR"
        echo ""
        echo "Please build for rg35xx first:"
        echo "  make PLATFORM=rg35xx build"
        echo ""
        exit 1
    fi
    
    # Determine which binary to run
    case "$BINARY" in
        minui)
            BINARY_PATH="$BIN_DIR/minui.elf"
            ;;
        minarch)
            BINARY_PATH="$BIN_DIR/minarch.elf"
            ;;
        *)
            echo "ERROR: Unknown binary '$BINARY'"
            echo "Usage: $0 [minui|minarch]"
            exit 1
            ;;
    esac
    
    # Verify binary exists
    if [ ! -f "$BINARY_PATH" ]; then
        echo "ERROR: Binary not found: $BINARY_PATH"
        echo ""
        echo "Please build for rg35xx first:"
        echo "  make PLATFORM=rg35xx build"
        echo "  make PLATFORM=rg35xx system"
        echo ""
        exit 1
    fi
    
    echo "Binary: $BINARY_PATH"
    echo "Sysroot: $BUILD_DIR"
    echo ""
    
    # Setup environment
    export LD_LIBRARY_PATH="$LIB_DIR:$LD_LIBRARY_PATH"
    
    # Display notes
    echo "==================================================="
    echo "Notes:"
    echo "  - Running ARM binary via user-mode emulation"
    echo "  - Graphics/display not available in user mode"
    echo "  - For full system emulation, see docs/DEV_QEMU.md"
    echo "  - Press Ctrl+C to exit"
    echo "==================================================="
    echo ""
    
    # Run the binary
    echo "Executing: $QEMU_ARM -L $BUILD_DIR $BINARY_PATH"
    echo ""
    
    # Use exec to replace the shell with qemu
    exec "$QEMU_ARM" -L "$BUILD_DIR" "$BINARY_PATH" "$@"
}

main "$@"
