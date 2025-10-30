#!/bin/bash
# Test script for minarch with gambatte core
# Usage: ./test_minarch.sh [path_to_rom]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEV_DIR="$(dirname "$SCRIPT_DIR")"
CORES_DIR="$DEV_DIR/cores/output"
MINARCH_DIR="$DEV_DIR/../all/minarch/build/dev"

# Core and ROM paths
CORE_PATH="${CORES_DIR}/gambatte_libretro.so"
ROM_PATH="${1:-/tmp/minui_dev/Roms/GB/test.gb}"

# Check if core exists
if [ ! -f "$CORE_PATH" ]; then
    echo "Error: Core not found at $CORE_PATH"
    echo "Please build the core first:"
    echo "  cd $DEV_DIR/cores && PLATFORM=dev make gambatte"
    exit 1
fi

# Check if minarch exists
if [ ! -f "$MINARCH_DIR/minarch.elf" ]; then
    echo "Error: minarch.elf not found at $MINARCH_DIR/minarch.elf"
    echo "Please build minarch first:"
    echo "  cd $DEV_DIR && export CROSS_COMPILE=' ' && PLATFORM=dev make minarch"
    exit 1
fi

# Check if ROM exists
if [ ! -f "$ROM_PATH" ]; then
    echo "Error: ROM not found at $ROM_PATH"
    echo ""
    echo "Please place a Game Boy ROM at:"
    echo "  /tmp/minui_dev/Roms/GB/test.gb"
    echo ""
    echo "Or provide a path as an argument:"
    echo "  $0 /path/to/your/rom.gb"
    echo ""
    echo "You can use homebrew ROMs from:"
    echo "  - https://github.com/gbdev/awesome-gbdev#games"
    echo "  - https://itch.io/games/tag-game-boy"
    exit 1
fi

echo "Starting minarch with gambatte core..."
echo "  Core: $CORE_PATH"
echo "  ROM:  $ROM_PATH"
echo ""
echo "Controls:"
echo "  Arrow keys: D-Pad"
echo "  X: A button"
echo "  Z: B button"
echo "  Enter: Start"
echo "  Right Shift: Select"
echo "  Escape: Menu"
echo ""

# Run minarch
exec "$MINARCH_DIR/minarch.elf" "$CORE_PATH" "$ROM_PATH"
