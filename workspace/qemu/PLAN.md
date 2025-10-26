# QEMU Backend Development Plan

This document outlines the roadmap for QEMU-based development and testing infrastructure for MinUI.

## Current Status (This PR)

✅ **Quick Dev Workflow (User-Mode Emulation)**

This PR implements a minimal, safe-first approach:
- `scripts/qemu/run-qemu-user.sh` - Run ARM binaries via qemu-user
- `docs/DEV_QEMU.md` - Documentation for the quick dev workflow  
- `make qemu-run` - Simple make target for developers
- Reuses existing `build/SYSTEM/rg35xx` tree from normal builds
- Works on macOS (M1/ARM64) and Linux hosts
- Non-destructive to existing build targets and toolchains

**Limitations of user-mode:**
- No graphics/framebuffer
- No input devices
- No hardware peripherals
- Limited to testing business logic and library dependencies

## Future Work

### Phase 1: Full System Emulation Foundation

**Goal:** Run MinUI with graphics and input in QEMU system emulator

Tasks:
- [ ] Research and select appropriate QEMU machine type (e.g., `virt`, `raspi3`, or custom)
- [ ] Acquire or build a compatible ARM Linux kernel
  - Option A: Use stock upstream kernel with generic ARM configs
  - Option B: Use vendor kernel from Anbernic/device SDK
  - Option C: Build custom minimal kernel for QEMU
- [ ] Create or obtain device tree blobs (DTB) for the selected machine
- [ ] Build a bootable disk image with:
  - Kernel
  - Initramfs or rootfs
  - MinUI binaries and assets
  - Framebuffer device support
  - Input device emulation (keyboard mapped to gamepad)

**Deliverables:**
- `scripts/qemu/build-qemu-image.sh` - Create bootable QEMU images
- `scripts/qemu/run-qemu-system.sh` - Launch full system emulator
- Updated documentation in `docs/DEV_QEMU.md`

### Phase 2: Developer Experience Improvements

**Goal:** Make QEMU workflow seamless for contributors

Tasks:
- [ ] Add `make qemu-build` target to create QEMU images from scratch
- [ ] Add `make qemu-install` to install/update MinUI in running QEMU image
- [ ] Implement shared folder between host and QEMU (virtio-9p or similar)
  - Auto-sync ROM directories
  - Share build outputs
- [ ] Create keyboard mapping configuration for MinUI controls
- [ ] Add snapshot/restore support for quick iteration
- [ ] Optimize boot time (skip unnecessary init services)

**Deliverables:**
- Enhanced makefile targets
- Configuration files for input mapping
- Documentation for rapid development workflow

### Phase 3: Multi-Device Profiles

**Goal:** Support multiple MinUI platforms in QEMU

Tasks:
- [ ] Create device profiles for different platforms:
  - rg35xx (current focus)
  - miyoomini
  - trimuismart
  - Other popular devices
- [ ] Device-specific configurations:
  - Screen resolution and orientation
  - Button layouts
  - Performance characteristics (CPU speed, memory)
- [ ] Profile switcher in QEMU scripts

**Deliverables:**
- `workspace/qemu/profiles/` directory with device configs
- Updated scripts to select device profile
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
