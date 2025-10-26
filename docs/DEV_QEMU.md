# QEMU Development Workflow

This document describes how to use QEMU for quick development and testing of MinUI on a developer workstation without requiring physical hardware.

## Quick Dev Workflow (User-Mode Emulation)

The simplest way to test MinUI changes is to use QEMU's user-mode emulation. This runs individual ARM binaries from an existing `rg35xx` build on your development machine.

### Prerequisites

#### macOS
```bash
brew install qemu
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt install qemu-user-static
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install qemu-user-static
```

### Build and Run

1. **Build for rg35xx platform:**
   ```bash
   make PLATFORM=rg35xx build
   make PLATFORM=rg35xx system
   ```

2. **Run MinUI under QEMU:**
   ```bash
   make qemu-run
   ```
   
   Or run the script directly:
   ```bash
   ./scripts/qemu/run-qemu-user.sh minui
   ```

3. **Run Minarch under QEMU:**
   ```bash
   ./scripts/qemu/run-qemu-user.sh minarch
   ```

### How It Works

The `run-qemu-user.sh` script:
- Detects `qemu-arm` or `qemu-arm-static` on your system
- Verifies that `build/SYSTEM/rg35xx` exists
- Sets up library paths pointing to the rg35xx sysroot
- Executes the ARM binary using `qemu-arm -L` with the sysroot

User-mode QEMU translates ARM syscalls to your host OS on-the-fly, allowing ARM binaries to run directly without a full system emulator.

### Limitations

**User-mode emulation has several limitations:**

- **No graphics:** There's no framebuffer device, so MinUI's display won't work
- **No input devices:** Keyboard/gamepad input won't be available
- **Limited hardware access:** GPIO, audio, and other hardware peripherals won't function
- **Testing scope:** Best for testing business logic, file I/O, and crash debugging

For these reasons, user-mode emulation is primarily useful for:
- Quick syntax/compilation checks
- Testing non-UI code paths
- Running under debuggers like `gdb`
- Validating library dependencies load correctly

## Full System Emulation (Future)

For complete hardware emulation including graphics, input, and peripherals, a full QEMU system emulator is needed. This requires:

1. A compatible ARM kernel (Linux kernel for the target SoC)
2. Device tree blobs (DTB) matching the hardware
3. A root filesystem image
4. QEMU machine configuration for the specific SoC

See `workspace/qemu/PLAN.md` for the roadmap of full system emulation support.

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
