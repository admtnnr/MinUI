#!/bin/sh
#
# run-qemu-system.sh - Run MinUI in QEMU full system emulation
#
# This script launches a complete QEMU ARM virtual machine with framebuffer
# and input device support, enabling full MinUI testing including UI and controls.
#
# Usage:
#   ./scripts/qemu/run-qemu-system.sh [options]
#
# Options:
#   --debug         Enable QEMU debug output
#   --gdb           Start QEMU with GDB server on port 1234
#   --vnc           Use VNC instead of SDL for display
#
# Prerequisites:
#   - QEMU system emulation (qemu-system-arm or qemu-system-aarch64)
#   - Disk image (run: ./scripts/qemu/build-qemu-image.sh)
#   - Kernel image in build/qemu/
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
QEMU_DIR="$REPO_ROOT/build/qemu"
IMAGE_FILE="$QEMU_DIR/minui-rg35xx.img"
KERNEL_FILE="$QEMU_DIR/zImage"

# Default settings
MEMORY="256M"
CPU="cortex-a7"
MACHINE="virt"
DEBUG_MODE=0
GDB_MODE=0
VNC_MODE=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    echo "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo "${RED}[ERROR]${NC} $1" >&2
}

# Parse command line arguments
parse_args() {
    while [ $# -gt 0 ]; do
        case "$1" in
            --debug)
                DEBUG_MODE=1
                shift
                ;;
            --gdb)
                GDB_MODE=1
                shift
                ;;
            --vnc)
                VNC_MODE=1
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Show help
show_help() {
    cat << EOF
Usage: $0 [options]

Run MinUI in QEMU full system emulation with framebuffer and input support.

Options:
  --debug         Enable QEMU debug output
  --gdb           Start QEMU with GDB server on port 1234
  --vnc           Use VNC instead of SDL for display
  -h, --help      Show this help message

Prerequisites:
  1. Build the disk image:
     ./scripts/qemu/build-qemu-image.sh
  
  2. Provide a kernel:
     Download or build an ARM kernel and place at:
     $KERNEL_FILE

  3. Install QEMU:
     macOS: brew install qemu
     Linux: sudo apt install qemu-system-arm

Example:
  $0
  $0 --debug
  $0 --gdb
EOF
}

# Find QEMU binary
find_qemu() {
    # Try different QEMU variants
    if command -v qemu-system-arm >/dev/null 2>&1; then
        echo "qemu-system-arm"
    elif command -v qemu-system-aarch64 >/dev/null 2>&1; then
        echo "qemu-system-aarch64"
    else
        echo ""
    fi
}

# Check prerequisites
check_prerequisites() {
    info "Checking prerequisites..."
    
    # Check for QEMU
    QEMU_BIN="$(find_qemu)"
    if [ -z "$QEMU_BIN" ]; then
        error "QEMU system emulator not found"
        error ""
        error "Install QEMU:"
        error "  macOS:  brew install qemu"
        error "  Linux:  sudo apt install qemu-system-arm"
        exit 1
    fi
    info "Found QEMU: $QEMU_BIN"
    
    # Check for disk image
    if [ ! -f "$IMAGE_FILE" ]; then
        error "Disk image not found: $IMAGE_FILE"
        error ""
        error "Build the image first:"
        error "  ./scripts/qemu/build-qemu-image.sh"
        exit 1
    fi
    info "Found disk image: $IMAGE_FILE"
    
    # Check for kernel
    if [ ! -f "$KERNEL_FILE" ]; then
        warn "Kernel not found: $KERNEL_FILE"
        warn ""
        warn "You need to provide an ARM kernel for QEMU."
        warn "Quick option (generic ARM kernel):"
        warn "  mkdir -p $QEMU_DIR"
        warn "  wget https://github.com/dhruvvyas90/qemu-rpi-kernel/raw/master/kernel-qemu-4.4.34-jessie -O $KERNEL_FILE"
        warn ""
        error "Cannot continue without kernel"
        exit 1
    fi
    info "Found kernel: $KERNEL_FILE"
    
    info "Prerequisites check passed"
}

# Build QEMU command line
build_qemu_cmd() {
    CMD="$QEMU_BIN"
    
    # Machine and CPU
    CMD="$CMD -M $MACHINE"
    CMD="$CMD -cpu $CPU"
    CMD="$CMD -m $MEMORY"
    
    # Kernel and root device
    CMD="$CMD -kernel $KERNEL_FILE"
    CMD="$CMD -drive file=$IMAGE_FILE,format=raw,if=virtio"
    CMD="$CMD -append 'root=/dev/vda rw console=ttyAMA0 init=/init'"
    
    # Display options
    if [ $VNC_MODE -eq 1 ]; then
        CMD="$CMD -vnc :0"
        info "VNC server will be available on localhost:5900"
    else
        # SDL display with framebuffer
        CMD="$CMD -serial stdio"
        # Use virtio-gpu for better graphics performance
        CMD="$CMD -device virtio-gpu-pci"
    fi
    
    # Input devices
    # Add USB keyboard and mouse
    CMD="$CMD -device qemu-xhci"
    CMD="$CMD -device usb-kbd"
    CMD="$CMD -device usb-mouse"
    
    # Add gamepad/joystick emulation
    # Map keyboard to gamepad buttons for MinUI controls
    CMD="$CMD -device usb-tablet"
    
    # Network (optional, for future enhancements)
    CMD="$CMD -netdev user,id=net0 -device virtio-net-pci,netdev=net0"
    
    # Debug and development options
    if [ $DEBUG_MODE -eq 1 ]; then
        CMD="$CMD -d guest_errors"
    fi
    
    if [ $GDB_MODE -eq 1 ]; then
        CMD="$CMD -s -S"
        info "GDB server enabled on port 1234"
        info "Connect with: arm-linux-gnueabihf-gdb"
        info "  (gdb) target remote :1234"
        info "  (gdb) continue"
    fi
    
    echo "$CMD"
}

# Main execution
main() {
    parse_args "$@"
    
    echo "========================================================"
    echo "MinUI QEMU Full System Emulation"
    echo "========================================================"
    echo ""
    
    check_prerequisites
    
    QEMU_CMD="$(build_qemu_cmd)"
    
    echo ""
    echo "========================================================"
    info "Configuration:"
    echo "  Machine:     $MACHINE"
    echo "  CPU:         $CPU"
    echo "  Memory:      $MEMORY"
    echo "  Disk Image:  $IMAGE_FILE"
    echo "  Kernel:      $KERNEL_FILE"
    if [ $VNC_MODE -eq 1 ]; then
        echo "  Display:     VNC (localhost:5900)"
    else
        echo "  Display:     SDL (native window)"
    fi
    echo ""
    info "Starting QEMU..."
    echo "========================================================"
    echo ""
    
    # Display keyboard mappings
    info "Keyboard Controls:"
    echo "  Arrow Keys  = D-Pad"
    echo "  Z           = A Button"
    echo "  X           = B Button"
    echo "  A           = X Button"
    echo "  S           = Y Button"
    echo "  Q           = L Trigger"
    echo "  W           = R Trigger"
    echo "  Enter       = Start"
    echo "  Space       = Select"
    echo "  Esc         = Menu"
    echo ""
    echo "  Ctrl+Alt+G  = Release mouse cursor"
    echo "  Ctrl+Alt+F  = Toggle fullscreen"
    echo "  Ctrl+Alt+Q  = Quit QEMU"
    echo ""
    
    if [ $DEBUG_MODE -eq 1 ]; then
        info "Debug mode enabled"
        info "QEMU command: $QEMU_CMD"
        echo ""
    fi
    
    # Execute QEMU
    exec $QEMU_CMD
}

main "$@"
