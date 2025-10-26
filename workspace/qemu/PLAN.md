# QEMU Backend Development Plan

This document outlines the roadmap for QEMU-based development and testing infrastructure for MinUI.

## Current Status (Implemented)

✅ **User-Mode Emulation (Quick Dev)**
- `scripts/qemu/run-qemu-user.sh` - Run ARM binaries via qemu-user
- `make qemu-user` - Quick testing without graphics
- Limitations: No framebuffer, no input devices, no hardware peripherals

✅ **Full System Emulation (Phase 1 - COMPLETE)**
- `scripts/qemu/build-qemu-image.sh` - Create bootable QEMU disk images
- `scripts/qemu/run-qemu-system.sh` - Launch full system emulator
- `make qemu-build-image` - Build disk image with MinUI
- `make qemu-system` / `make qemu-run` - Run with framebuffer and input support
- Framebuffer device support (`/dev/fb0`)
- Input device emulation (keyboard mapped to gamepad)
- virtio-gpu for graphics
- USB input devices (keyboard, mouse, gamepad)
- Complete documentation in `docs/DEV_QEMU.md`

**Key Features:**
- Works on macOS (M1/ARM64) and Linux hosts
- Reuses existing `build/SYSTEM/rg35xx` tree from normal builds
- Non-destructive to existing build targets and toolchains
- Supports UI testing, cheat support, and full feature testing
- Keyboard controls mapped to gamepad buttons

## Completed Work

### Phase 1: Full System Emulation Foundation ✅

**Goal:** Run MinUI with graphics and input in QEMU system emulator

Completed Tasks:
- [x] Selected QEMU machine type (`virt` with ARM Cortex-A7)
- [x] Documented kernel options (generic ARM kernel recommended)
- [x] Created bootable disk image builder
- [x] Built init script with framebuffer and input device setup
- [x] Implemented framebuffer device support
- [x] Implemented input device emulation (keyboard to gamepad mapping)
- [x] Added virtio-gpu for graphics
- [x] Added USB input devices
- [x] Updated makefile with qemu-system targets
- [x] Comprehensive documentation

**Deliverables:**
- ✅ `scripts/qemu/build-qemu-image.sh` - Create bootable QEMU images
- ✅ `scripts/qemu/run-qemu-system.sh` - Launch full system emulator
- ✅ Updated documentation in `docs/DEV_QEMU.md`
- ✅ Makefile targets: `qemu-build-image`, `qemu-system`, `qemu-run`

## Future Work

### Phase 2: Developer Experience Improvements

**Goal:** Make QEMU workflow seamless for contributors

Tasks:
- [ ] Implement hot-reload: automatically rebuild and restart QEMU on code changes
- [ ] Add `make qemu-install` to update MinUI in running QEMU image without full rebuild
- [ ] Implement shared folder between host and QEMU (virtio-9p or similar)
  - Auto-sync ROM directories
  - Share build outputs
- [ ] Optimize boot time (skip unnecessary init services, use minimal kernel)
- [ ] Add snapshot/restore support for quick iteration
- [ ] Create automated test runner in QEMU
- [ ] Prebuilt kernel images for common configurations

**Deliverables:**
- Enhanced makefile targets for rapid development
- Shared folder configuration
- Optimized boot configuration
- Snapshot management scripts

### Phase 3: Multi-Device Profiles

**Goal:** Support multiple MinUI platforms in QEMU

Tasks:
- [ ] Create device profiles for different platforms:
  - rg35xx (640x480) - current implementation
  - miyoomini (640x480)
  - trimuismart (320x240)
  - rg35xxplus (640x480)
  - Other popular devices
- [ ] Device-specific configurations:
  - Screen resolution and orientation
  - Button layouts and mappings
  - Performance characteristics (CPU speed, memory)
- [ ] Profile switcher: `make qemu-run PROFILE=miyoomini`
- [ ] Profile validation and testing

**Deliverables:**
- `workspace/qemu/profiles/` directory with device configs
- Updated scripts to select device profile at runtime
- Documentation for each profile
- Testing matrix for cross-device compatibility

### Phase 4: CI/CD Integration

**Goal:** Automated testing in CI pipelines

Tasks:
- [ ] Headless QEMU execution (no graphics output needed)
- [ ] Automated test suite running in QEMU
  - Boot test
  - Menu navigation test
  - ROM loading test
  - Screenshot comparison tests
- [ ] GitHub Actions workflow integration
- [ ] Nightly builds with QEMU smoke tests

**Deliverables:**
- `.github/workflows/qemu-test.yml`
- Test scripts and fixtures
- Automated regression detection

### Phase 5: Docker/Containerization (Optional)

**Goal:** Standardize QEMU environment across platforms

Tasks:
- [ ] Create Docker image with:
  - QEMU installation
  - ARM cross-compilation toolchain
  - MinUI build dependencies
  - Pre-built kernels and images
- [ ] Docker Compose configuration for development
- [ ] Instructions for Docker-based workflow

**Deliverables:**
- `Dockerfile.qemu`
- `docker-compose.yml`
- Documentation for Docker workflow

## Technical Considerations

### Kernel Selection

**Options:**
1. **Generic ARM kernel** - Most portable, easiest to build
2. **Vendor kernel** - Most accurate hardware emulation
3. **Custom minimal kernel** - Fastest boot, smallest size

Recommendation: Start with generic ARM kernel (buildroot or Debian armhf), then explore vendor kernels if hardware accuracy is needed.

### Machine Type

**QEMU ARM machine types to consider:**
- `virt` - Generic ARM virtual machine (most flexible)
- `raspi2` / `raspi3` - Raspberry Pi (well-documented)
- Custom - Define new machine type (advanced)

Recommendation: Use `virt` machine type with appropriate CPU (cortex-a7 or cortex-a9).

### Graphics

**Options:**
1. **virtio-gpu** - Modern 3D-capable virtual GPU
2. **Simple framebuffer** - Basic 2D framebuffer device
3. **SDL/VNC output** - Direct window or remote display

Recommendation: Simple framebuffer mapped to SDL window for development.

### Storage

**Options:**
1. **Disk image** - Full system in a .img file (portable)
2. **virtio-fs / 9p** - Shared directory with host (development)
3. **NBD** - Network block device (advanced)

Recommendation: Combination of disk image for system and virtio-9p for ROM/development directories.

## Resources and References

- QEMU User Space Emulator: https://qemu-project.gitlab.io/qemu/user/main.html
- QEMU System Emulator: https://qemu-project.gitlab.io/qemu/system/index.html
- Buildroot (for creating embedded Linux images): https://buildroot.org/
- Yocto Project: https://www.yoctoproject.org/
- MinUI Architecture: `docs/ARCHITECTURE.md`

## Success Criteria

A successful QEMU backend implementation will:
1. Allow developers to test MinUI without hardware
2. Support rapid iteration (< 10 second rebuild-test cycle)
3. Provide accurate-enough emulation for most features
4. Integrate into existing build system seamlessly
5. Work on macOS (M1/Intel), Linux, and Windows (WSL2)
6. Enable automated testing in CI/CD

## Timeline (Tentative)

- **Phase 1:** 2-4 weeks (kernel + basic system)
- **Phase 2:** 1-2 weeks (developer experience)
- **Phase 3:** 2-3 weeks (multi-device support)
- **Phase 4:** 1-2 weeks (CI/CD integration)
- **Phase 5:** 1 week (Docker, optional)

Total estimated effort: 7-12 weeks of focused development.

## Contributing

Interested in helping with QEMU backend development? 

1. Start with user-mode testing (this PR)
2. Experiment with QEMU system emulation locally
3. Share findings and configurations in GitHub issues
4. Submit PRs for incremental improvements

Questions? Open a discussion in the MinUI repository.
