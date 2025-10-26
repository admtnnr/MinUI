#!/bin/sh
#
# build-qemu-image.sh - Build a bootable QEMU system image for MinUI
#
# This script creates a disk image with a Linux rootfs that can boot
# in QEMU system emulation with framebuffer and input support.
#
# Usage:
#   ./scripts/qemu/build-qemu-image.sh
#
# Prerequisites:
#   - Build artifacts in build/SYSTEM/rg35xx (run: make PLATFORM=rg35xx build system)
#   - qemu-img (part of qemu package)
#   - Basic Linux utilities (dd, mkfs.ext4, mount, etc.)
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$REPO_ROOT/build/SYSTEM/rg35xx"
QEMU_DIR="$REPO_ROOT/build/qemu"
IMAGE_FILE="$QEMU_DIR/minui-rg35xx.img"
IMAGE_SIZE="512M"
MOUNT_POINT="/tmp/minui-qemu-mount"

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

# Check prerequisites
check_prerequisites() {
    info "Checking prerequisites..."
    
    # Check for build directory
    if [ ! -d "$BUILD_DIR" ]; then
        error "Build directory not found: $BUILD_DIR"
        error "Please build for rg35xx first:"
        error "  make PLATFORM=rg35xx build"
        error "  make PLATFORM=rg35xx system"
        exit 1
    fi
    
    # Check for qemu-img
    if ! command -v qemu-img >/dev/null 2>&1; then
        error "qemu-img not found. Please install QEMU:"
        error "  macOS: brew install qemu"
        error "  Linux: sudo apt install qemu-utils"
        exit 1
    fi
    
    # Check if running as root (needed for mounting)
    if [ "$(id -u)" -ne 0 ]; then
        warn "This script needs root privileges to mount filesystems"
        warn "You may be prompted for your password"
    fi
    
    info "Prerequisites check passed"
}

# Create disk image
create_disk_image() {
    info "Creating disk image: $IMAGE_FILE (${IMAGE_SIZE})"
    
    mkdir -p "$QEMU_DIR"
    
    # Create raw disk image
    qemu-img create -f raw "$IMAGE_FILE" "$IMAGE_SIZE"
    
    # Create ext4 filesystem
    info "Creating ext4 filesystem..."
    sudo mkfs.ext4 -F "$IMAGE_FILE"
    
    info "Disk image created successfully"
}

# Mount image and populate with rootfs
populate_rootfs() {
    info "Populating rootfs from $BUILD_DIR"
    
    # Create mount point
    sudo mkdir -p "$MOUNT_POINT"
    
    # Mount the image
    info "Mounting image..."
    sudo mount -o loop "$IMAGE_FILE" "$MOUNT_POINT"
    
    # Ensure unmount on exit
    trap 'sudo umount "$MOUNT_POINT" 2>/dev/null || true; sudo rmdir "$MOUNT_POINT" 2>/dev/null || true' EXIT
    
    # Copy basic directory structure
    info "Creating directory structure..."
    sudo mkdir -p "$MOUNT_POINT/bin"
    sudo mkdir -p "$MOUNT_POINT/lib"
    sudo mkdir -p "$MOUNT_POINT/usr/lib"
    sudo mkdir -p "$MOUNT_POINT/dev"
    sudo mkdir -p "$MOUNT_POINT/proc"
    sudo mkdir -p "$MOUNT_POINT/sys"
    sudo mkdir -p "$MOUNT_POINT/tmp"
    sudo mkdir -p "$MOUNT_POINT/etc"
    sudo mkdir -p "$MOUNT_POINT/mnt"
    sudo mkdir -p "$MOUNT_POINT/root"
    sudo mkdir -p "$MOUNT_POINT/home"
    sudo mkdir -p "$MOUNT_POINT/minui"
    
    # Copy MinUI binaries and libraries
    info "Copying MinUI binaries..."
    sudo cp -r "$BUILD_DIR/bin" "$MOUNT_POINT/minui/"
    
    if [ -d "$BUILD_DIR/lib" ]; then
        info "Copying libraries..."
        sudo cp -r "$BUILD_DIR/lib" "$MOUNT_POINT/minui/"
    fi
    
    if [ -d "$BUILD_DIR/cores" ]; then
        info "Copying cores..."
        sudo cp -r "$BUILD_DIR/cores" "$MOUNT_POINT/minui/"
    fi
    
    # Create basic init script
    info "Creating init script..."
    sudo tee "$MOUNT_POINT/init" > /dev/null << 'EOF'
#!/bin/sh
# MinUI QEMU Init Script

echo "MinUI QEMU System Starting..."

# Mount essential filesystems
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /tmp

# Set up library path
export LD_LIBRARY_PATH=/minui/lib:/lib:/usr/lib

# Create basic device nodes if they don't exist
[ -e /dev/null ] || mknod -m 666 /dev/null c 1 3
[ -e /dev/zero ] || mknod -m 666 /dev/zero c 1 5
[ -e /dev/random ] || mknod -m 666 /dev/random c 1 8
[ -e /dev/urandom ] || mknod -m 666 /dev/urandom c 1 9

# Set up framebuffer if available
if [ -e /dev/fb0 ]; then
    echo "Framebuffer device detected: /dev/fb0"
    chmod 666 /dev/fb0
fi

# Set up input devices
for input in /dev/input/*; do
    [ -e "$input" ] && chmod 666 "$input"
done

echo "Starting MinUI..."
cd /minui

# Run MinUI
if [ -x /minui/bin/minui.elf ]; then
    exec /minui/bin/minui.elf
else
    echo "ERROR: MinUI binary not found or not executable"
    echo "Available files in /minui/bin:"
    ls -la /minui/bin/ || true
    exec /bin/sh
fi
EOF
    
    sudo chmod +x "$MOUNT_POINT/init"
    
    # Create minimal inittab for BusyBox
    sudo tee "$MOUNT_POINT/etc/inittab" > /dev/null << 'EOF'
::sysinit:/init
::respawn:/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
EOF
    
    # Set permissions
    sudo chmod 755 "$MOUNT_POINT"
    sudo chmod 1777 "$MOUNT_POINT/tmp"
    
    info "Rootfs populated successfully"
    
    # Unmount
    info "Unmounting image..."
    sudo umount "$MOUNT_POINT"
    sudo rmdir "$MOUNT_POINT"
    
    # Clear trap
    trap - EXIT
}

# Download and setup minimal kernel (if needed)
setup_kernel() {
    info "Setting up kernel for QEMU..."
    
    KERNEL_FILE="$QEMU_DIR/zImage"
    
    # Check if kernel already exists
    if [ -f "$KERNEL_FILE" ]; then
        warn "Kernel already exists: $KERNEL_FILE"
        warn "Skipping kernel download"
        return
    fi
    
    # For now, we'll note that the kernel should be provided
    # In a real implementation, we might download a prebuilt kernel or build one
    warn "Kernel not found. You need to provide a kernel for QEMU."
    warn "Options:"
    warn "  1. Download a prebuilt ARM kernel and place it at: $KERNEL_FILE"
    warn "  2. Build a custom kernel with framebuffer and input support"
    warn "  3. Use a kernel from the device vendor/SDK"
    warn ""
    warn "For testing, you can use a generic ARM virt kernel:"
    warn "  wget https://github.com/dhruvvyas90/qemu-rpi-kernel/raw/master/kernel-qemu-4.4.34-jessie -O $KERNEL_FILE"
}

# Main execution
main() {
    echo "========================================================"
    echo "MinUI QEMU System Image Builder"
    echo "========================================================"
    echo ""
    
    check_prerequisites
    create_disk_image
    populate_rootfs
    setup_kernel
    
    echo ""
    echo "========================================================"
    info "Image build complete!"
    echo ""
    info "Disk image: $IMAGE_FILE"
    info "Next steps:"
    echo "  1. Provide a kernel at: $QEMU_DIR/zImage"
    echo "  2. Run the system: ./scripts/qemu/run-qemu-system.sh"
    echo "     or: make qemu-system"
    echo "========================================================"
}

main "$@"
