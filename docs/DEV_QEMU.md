# QEMU Development Workflow

This document describes how to use QEMU for development and testing of MinUI on a developer workstation without requiring physical hardware.

## Full System Emulation (Recommended)

The recommended way to test MinUI is using QEMU's full system emulation, which provides framebuffer and input device support. This enables complete testing including UI, controls, and features like cheat support.

### Prerequisites

#### macOS
```bash
brew install qemu
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt install qemu-system-arm qemu-utils
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install qemu-system-arm qemu-img
```

### Build and Run

1. **Build for rg35xx platform:**
   ```bash
   make PLATFORM=rg35xx build
   make PLATFORM=rg35xx system
   ```

2. **Build QEMU disk image (first time only):**
   ```bash
   make qemu-build-image
   ```
   
   This creates a bootable disk image with MinUI at `build/qemu/minui-rg35xx.img`.

3. **Provide a kernel (first time only):**
   
   Download a generic ARM kernel:
   ```bash
   mkdir -p build/qemu
   wget https://github.com/dhruvvyas90/qemu-rpi-kernel/raw/master/kernel-qemu-4.4.34-jessie \
     -O build/qemu/zImage
   ```
   
   Or use a custom kernel built with framebuffer and input support.

4. **Run MinUI in QEMU:**
   ```bash
   make qemu-run
   ```
   
   Or run the script directly:
   ```bash
   ./scripts/qemu/run-qemu-system.sh
   ```

### Keyboard Controls in QEMU

When running in QEMU, keyboard keys are mapped to gamepad controls:

| Keyboard | MinUI Control |
|----------|---------------|
| Arrow Keys | D-Pad |
| Z | A Button |
| X | B Button |
| A | X Button |
| S | Y Button |
| Q | L Trigger |
| W | R Trigger |
| Enter | Start |
| Space | Select |
| Esc | Menu |

QEMU window controls:
- `Ctrl+Alt+G` - Release mouse cursor
- `Ctrl+Alt+F` - Toggle fullscreen
- `Ctrl+Alt+Q` - Quit QEMU

### How It Works

The full system emulation:
- Uses `qemu-system-arm` to emulate a complete ARM virtual machine
- Boots a Linux kernel with framebuffer and input support
- Mounts the disk image containing MinUI binaries and libraries
- Provides virtio-gpu for graphics output
- Emulates USB keyboard, mouse, and gamepad input
- Creates device nodes (`/dev/fb0`, `/dev/input/*`) that MinUI can access

This provides the most complete testing environment, supporting all MinUI features including UI rendering, input handling, and hardware-dependent features.

### Advanced Options

**Debug mode:**
```bash
./scripts/qemu/run-qemu-system.sh --debug
```

**GDB debugging:**
```bash
# Terminal 1: Start QEMU with GDB server
./scripts/qemu/run-qemu-system.sh --gdb

# Terminal 2: Connect with GDB
arm-linux-gnueabihf-gdb build/SYSTEM/rg35xx/bin/minui.elf
(gdb) target remote :1234
(gdb) continue
```

**VNC display (headless):**
```bash
./scripts/qemu/run-qemu-system.sh --vnc
# Connect VNC viewer to localhost:5900
```

## User-Mode Emulation (Legacy)

For quick testing without graphics, you can use user-mode emulation. This has significant limitations but may be useful for testing non-UI code.

### Prerequisites

#### macOS
```bash
brew install qemu
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt install qemu-user-static
```

### Build and Run
1. **Build for rg35xx platform:**
   ```bash
   make PLATFORM=rg35xx build
   make PLATFORM=rg35xx system
   ```

2. **Run MinUI under QEMU (user-mode):**
   ```bash
   make qemu-user
   ```
   
   Or run the script directly:
   ```bash
   ./scripts/qemu/run-qemu-user.sh minui
   ```

3. **Run Minarch under QEMU:**
   ```bash
   ./scripts/qemu/run-qemu-user.sh minarch
   ```

### How It Works (User-Mode)

The `run-qemu-user.sh` script:
- Detects `qemu-arm` or `qemu-arm-static` on your system
- Verifies that `build/SYSTEM/rg35xx` exists
- Sets up library paths pointing to the rg35xx sysroot
- Executes the ARM binary using `qemu-arm -L` with the sysroot

User-mode QEMU translates ARM syscalls to your host OS on-the-fly, allowing ARM binaries to run directly without a full system emulator.

### Limitations (User-Mode)

**User-mode emulation has several limitations:**

- **No graphics:** There's no framebuffer device, so MinUI's display won't work
- **No input devices:** Keyboard/gamepad input won't be available
- **Limited hardware access:** GPIO, audio, and other hardware peripherals won't function
- **Testing scope:** Best for testing business logic, file I/O, and crash debugging

**For complete testing with UI and input, use full system emulation instead (see above).**

For these reasons, user-mode emulation is primarily useful for:
- Quick syntax/compilation checks
- Testing non-UI code paths
- Running under debuggers like `gdb`
- Validating library dependencies load correctly

## Comparison: System vs User-Mode

| Feature | Full System | User-Mode |
|---------|-------------|-----------|
| Framebuffer/Graphics | ✅ Yes | ❌ No |
| Input Devices | ✅ Yes | ❌ No |
| UI Testing | ✅ Full | ❌ None |
| Cheat Support | ✅ Yes | ❌ No |
| Boot Time | ~5-10 sec | Instant |
| Setup Complexity | Medium | Low |
| Best For | Complete testing | Quick checks |

**Recommendation:** Use full system emulation for development and testing. Use user-mode only for quick non-UI testing.

## Rebuilding After Code Changes

After making changes to MinUI source code:

1. **Rebuild the platform:**
   ```bash
   make PLATFORM=rg35xx build
   make PLATFORM=rg35xx system
   ```

2. **Rebuild QEMU image:**
   ```bash
   make qemu-build-image
   ```

3. **Run in QEMU:**
   ```bash
   make qemu-run
   ```

Tip: You can combine steps 1-2:
```bash
make PLATFORM=rg35xx build system && make qemu-build-image && make qemu-run
```

## Troubleshooting

### "qemu-arm not found"

Install QEMU using the package manager instructions above. After installation, verify:

```bash
which qemu-arm        # or qemu-arm-static
qemu-arm --version
```

### "Build directory not found"

You need to build the rg35xx platform first:

```bash
make PLATFORM=rg35xx build
make PLATFORM=rg35xx system
```

This creates `build/SYSTEM/rg35xx/` with the necessary binaries and libraries.

### "Exec format error" or "Invalid ELF"

This usually means:
1. The binary wasn't built for ARM (check your build logs)
2. QEMU isn't properly installed
3. The binary is corrupted

Rebuild from scratch:
```bash
make clean
make PLATFORM=rg35xx build
make PLATFORM=rg35xx system
```

### Binary runs but crashes immediately

Check library dependencies:
```bash
qemu-arm -L build/SYSTEM/rg35xx build/SYSTEM/rg35xx/bin/minui.elf
```

Look for "library not found" errors. The rg35xx build should include all necessary libraries in `build/SYSTEM/rg35xx/lib/`.

### Running under GDB

For debugging, use QEMU's gdb stub:

```bash
# Terminal 1: Start QEMU with gdb server
qemu-arm -g 1234 -L build/SYSTEM/rg35xx build/SYSTEM/rg35xx/bin/minui.elf

# Terminal 2: Connect with arm-linux-gnueabihf-gdb
arm-linux-gnueabihf-gdb build/SYSTEM/rg35xx/bin/minui.elf
(gdb) target remote :1234
(gdb) continue
```

## Next Steps

- Implement full system QEMU machine configuration
- Create bootable disk images with MinUI pre-installed
- Add automated testing using QEMU in CI/CD
- Support for different device profiles (rg35xx, miyoomini, etc.)

See `workspace/qemu/PLAN.md` for detailed planning and future enhancements.
