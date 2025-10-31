# MinUI Dev Platform Testing Guide

## Overview

This guide covers testing strategies and workflows for the MinUI dev platform, including both the launcher (minui) and emulator frontend (minarch).

## Phase 3.5: MinArch Emulator Testing

### Prerequisites

1. **Build System**: Development environment with SDL2 installed
2. **Architecture**: Native builds (x86_64 or arm64)
3. **Test ROMs**: Legal ROMs you own or freely distributed homebrew

### Quick Start

```bash
# 1. Build everything
cd workspace/dev
export CROSS_COMPILE=" "
make PLATFORM=dev

# 2. Build gambatte core
cd cores
PLATFORM=dev make gambatte

# 3. Run unit tests
cd ../tests
make run

# 4. Test with a ROM (after placing it in /tmp/minui_dev/Roms/GB/)
./test_minarch.sh
```

## Recommended Test Plan

### Unit Tests

**Location**: `workspace/dev/tests/`

**Purpose**: Automated verification of core functionality

**Tests**:
1. **test_core_loading** - Verifies libretro cores can be loaded
   - Tests dlopen() functionality
   - Validates all required libretro API symbols
   - Extracts and displays core metadata

**Running unit tests**:
```bash
cd workspace/dev/tests
make clean && make run
```

**Expected output**: All tests should PASS (17/17 for gambatte)

### Integration Tests (Manual)

**Purpose**: Verify end-to-end emulator functionality

#### Test 1: Core Loading
```bash
cd workspace/dev/tests
./test_minarch.sh /path/to/rom.gb
```

**Success criteria**:
- ✅ minarch launches without errors
- ✅ SDL window appears
- ✅ No dlopen errors in output

#### Test 2: ROM Loading and Gameplay
**Success criteria**:
- ✅ ROM loads and displays graphics
- ✅ Input responds (D-pad, A/B buttons)
- ✅ Audio plays correctly
- ✅ No crashes during gameplay

#### Test 3: In-Game Menu
**Action**: Press ESC during gameplay

**Success criteria**:
- ✅ Menu appears overlay
- ✅ Can navigate menu with keyboard
- ✅ Options are readable and functional

#### Test 4: Save States
**Actions**:
1. Play game for a bit
2. Open menu (ESC)
3. Save state (slot 1)
4. Continue playing
5. Load state (slot 1)

**Success criteria**:
- ✅ Save state creates without errors
- ✅ Load state restores correct game position
- ✅ State file created in `/tmp/minui_dev/.userdata/dev/GB-gambatte/`

#### Test 5: Configuration Testing
**Configs to test**:
- `display_sharpness` (soft/sharp/crisp)
- `display_vsync` (on/off)
- `display_scale` (native/aspect/fullscreen)

**Method**: Edit `~/.minui_config` and restart minarch

**Success criteria**:
- ✅ Settings apply correctly
- ✅ Visual changes are noticeable
- ✅ No degradation in performance

### Python Automation Tests

**Location**: `workspace/dev/tools/`

**Purpose**: Automated screenshot-based regression testing

#### Future Test Cases (Phase 3.6+)

1. **test_boot_sequence.json**
   - Launch minarch
   - Capture screenshot after 2 seconds
   - Compare against golden image
   - Verify no crashes

2. **test_menu_navigation.json**
   - Input sequence: ESC, Down, Down, Enter
   - Capture screenshots at each step
   - Verify menu items match expected

3. **test_gameplay_stability.json**
   - Play for 60 seconds
   - Capture screenshot every 10 seconds
   - Verify no freezes or artifacts

**Note**: These will be implemented in Phase 3.6 once test ROMs are selected

## Debugging Workflows

### Debugging Core Loading Issues

```bash
# Enable verbose output
cd workspace/all/minarch/build/dev
LD_DEBUG=libs ./minarch.elf /path/to/core.so /path/to/rom.gb
```

### Debugging Graphics Issues

1. Check SDL2 is working:
```bash
sdl2-config --version
```

2. Test with different pixel formats in `platform.conf`:
```ini
[general]
pixel_format = RGB565  # Try ARGB8888 if issues
```

3. Enable debug overlay in minarch menu

### Debugging Input Issues

1. Verify SDL2 can detect input:
```bash
# Test keyboard
sdl2-jstest --list  # Should show any connected gamepads
```

2. Check input mapping in `platform.h`

3. Enable input event logging in `platform.c`

## Performance Testing

### Measuring Frame Rate

**In minarch menu**: Enable "Show FPS" option

**Expected results**:
- Game Boy games: 60 FPS constant
- No frame drops during normal gameplay
- Fast-forward: 2-4x speed depending on game

### Memory Profiling

```bash
# Build with address sanitizer (already enabled)
valgrind --leak-check=full ./minarch.elf core.so rom.gb
```

## CI/CD Integration

### GitHub Actions

**File**: `.github/workflows/test-dev-platform.yml`

**Tests run on every push**:
1. Build minui and minarch
2. Build gambatte core
3. Run unit tests
4. (Future) Run automated integration tests with test ROMs

**Artifacts uploaded**:
- Test binaries
- Core .so files
- Test reports
- Screenshots (future)

### Running CI Tests Locally

```bash
# Simulate CI environment
docker run -it --rm \
  -v $(pwd):/minui \
  -w /minui/workspace/dev \
  ubuntu:24.04 \
  bash -c "
    apt-get update &&
    apt-get install -y libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev build-essential git &&
    export CROSS_COMPILE=' ' &&
    make PLATFORM=dev &&
    cd cores && PLATFORM=dev make gambatte &&
    cd ../tests && make run
  "
```

## Test ROM Recommendations

### Gambatte (Game Boy)

**Homebrew ROMs** (free and legal):
- **Tobu Tobu Girl** - Puzzle platformer, tests physics and graphics
  - https://github.com/SimonLarsen/tobutobugirl/releases
- **µCity** - City sim, tests menu navigation and save states
  - https://github.com/AntonioND/ucity/releases
- **2048-gb** - Puzzle game, tests input responsiveness
  - https://github.com/Sanqui/2048-gb/releases
- **Infinity** - RPG, tests diverse gameplay mechanics
  - https://github.com/infinity-gbc/infinity/releases

**Why these ROMs**:
- ✅ Open source and freely distributable
- ✅ Actively maintained
- ✅ Cover different game genres
- ✅ Small file sizes (good for CI)
- ✅ Well-tested with Gambatte core

### Obtaining Test ROMs for CI

```bash
# Example: Download Tobu Tobu Girl
cd /tmp/minui_dev/Roms/GB
wget https://github.com/SimonLarsen/tobutobugirl/releases/download/v1.0/tobutobugirl.gb
```

**For CI**: Add download step to GitHub Actions workflow

## Test Coverage Goals

### Current Coverage (Phase 3.5)
- ✅ Core loading (unit tests)
- ✅ API symbol validation
- ✅ Build system verification
- ✅ Manual integration testing

### Future Coverage (Phase 3.6+)
- ⬜ Automated ROM loading tests
- ⬜ Screenshot-based regression tests
- ⬜ Save state integrity tests
- ⬜ Audio output validation
- ⬜ Performance benchmarks

## Troubleshooting

### "Core not found" error
**Solution**: Ensure core is built and path is correct
```bash
ls -la workspace/dev/cores/output/gambatte_libretro.so
```

### "ROM not found" error
**Solution**: Check ROM path and permissions
```bash
ls -la /tmp/minui_dev/Roms/GB/*.gb
```

### Black screen on launch
**Possible causes**:
1. SDL2 not initialized properly
2. ROM is corrupted
3. Core incompatible with ROM

**Debug**:
```bash
# Check SDL2
pkg-config --modversion sdl2

# Try different ROM
./test_minarch.sh /path/to/different/rom.gb
```

### Segmentation fault
**Possible causes**:
1. NULL pointer in core or minarch
2. Memory corruption
3. ABI mismatch

**Debug**:
```bash
# Run with gdb
gdb --args ./minarch.elf core.so rom.gb
(gdb) run
(gdb) bt  # If it crashes
```

## Best Practices

### 1. Always Run Unit Tests First
Before manual testing, verify basics:
```bash
cd workspace/dev/tests && make run
```

### 2. Test on Target Architecture
- Dev on M1 Mac: Ensure arm64 builds work
- CI testing: Test both x86_64 and arm64
- Use Docker for consistency

### 3. Use Small Test ROMs
- Faster iteration
- Easier to debug
- Better for CI/CD

### 4. Document Issues
When you find a bug:
1. Note reproduction steps
2. Capture screenshot if applicable
3. Check if it affects hardware platforms too
4. File issue with `[dev-platform]` tag

### 5. Keep Test ROMs Separate
```
/tmp/minui_dev/Roms/GB/
├── homebrew/        # Free ROMs for testing
└── test/            # Minimal test cases
```

## Next Steps

After Phase 3.5:
1. Select and document official test ROM set
2. Implement automated Python tests
3. Add more cores (fceumm for NES testing)
4. Create visual regression test suite
5. Performance benchmarking framework

## Resources

- [Libretro Docs](https://docs.libretro.com/)
- [Gambatte Core](https://github.com/libretro/gambatte-libretro)
- [GB Dev Resources](https://github.com/gbdev/awesome-gbdev)
- [SDL2 Documentation](https://wiki.libsdl.org/SDL2/)
