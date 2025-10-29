# Phase 3: Robust Testing Platform - Research & Implementation Plan

## Executive Summary

This document outlines the research findings and detailed implementation plan for Phase 3 of MinUI development, focusing on creating a robust testing infrastructure. The goal is to establish a full-featured, configurable platform for development and manual testing of minarch and minui, including UI interaction.

---

## 1. Current State Analysis

### 1.1 Existing Test Platform (`workspace/test/`)

**Status**: Minimal unit testing platform

**Capabilities**:
- ✅ Native x86_64 compilation with gcc + SDL2
- ✅ Config system unit tests (81 tests passing)
- ✅ No device hardware required
- ✅ Fast compile/test iteration

**Limitations**:
- ❌ Only tests config parsing (no UI/runtime testing)
- ❌ Minimal platform.h stub (only defines and test harness)
- ❌ Cannot run minui or minarch executables
- ❌ No input simulation
- ❌ No visual/graphics testing

**Files**:
```
workspace/test/
├── Makefile          # Simple gcc build for config tests
├── platform.h        # Minimal stubs (PLATFORM, SDCARD_PATH, BTN_ID_COUNT)
├── test_config.c     # 81 unit tests for config system
└── test_config       # Compiled test binary
```

**Purpose**: Automated unit testing of configuration system only

---

### 1.2 Existing MacOS Platform (`workspace/macos/`)

**Status**: Partially functional development platform

**Capabilities**:
- ✅ SDL2-based windowing and rendering
- ✅ Keyboard input mapping (arrows, WASD, etc.)
- ✅ Joystick support initialization
- ✅ Can compile and run minui executable
- ✅ Video renderer with texture streaming
- ✅ Fake SD card path (/Users/shauninman/Projects/Personal/MinUI/workspace/macos/FAKESD)

**Limitations**:
- ❌ Hardcoded for macOS/Homebrew paths
- ❌ Fixed 640x480 resolution
- ❌ No configuration options (screen size, buttons, etc.)
- ❌ Stubs for hardware features (battery, brightness, volume)
- ❌ Incomplete platform API implementation
- ❌ Not containerized or VM-ready
- ❌ No Phase 2 config system integration

**Implementation Details**:
- **Platform.c**: ~355 lines implementing PLAT_* functions
- **Video**: SDL2 window + renderer + texture (RGB565)
- **Input**: Keyboard scancodes + joystick (SDL_Joystick)
- **Stubs**: Battery (100%), brightness, volume, rumble, CPU speed
- **Screen**: Fixed 640x480, 16-bit color depth

**From notes.txt**:
> "This is not a full version of MinUI for macOS. It's a dummy platform to allow using more modern tools to compile and find issues in MinUI's source code without the roundtrip to a device."

**Purpose**: Compile-time debugging, not comprehensive testing

---

### 1.3 Real Hardware Platforms (Reference)

**Example**: rg35xxplus (999 lines), miyoomini (601 lines), rgb30 (760 lines)

**Full Platform Requirements** (from api.h):
```c
// Input
void PLAT_initInput(void);
void PLAT_quitInput(void);
void PLAT_pollInput(void);
int PLAT_shouldWake(void);
void PLAT_initLid(void);
int PLAT_lidChanged(int* state);

// Video
SDL_Surface* PLAT_initVideo(void);
void PLAT_quitVideo(void);
void PLAT_clearVideo(SDL_Surface* screen);
void PLAT_clearAll(void);
void PLAT_setVsync(int vsync);
SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch);
void PLAT_setVideoScaleClip(int x, int y, int width, int height);
void PLAT_setNearestNeighbor(int enabled);
void PLAT_setSharpness(int sharpness);
void PLAT_setEffectColor(int color);
void PLAT_setEffect(int effect);
void PLAT_vsync(int remaining);
scaler_t PLAT_getScaler(GFX_Renderer* renderer);
void PLAT_blitRenderer(GFX_Renderer* renderer);
void PLAT_flip(SDL_Surface* screen, int sync);
int PLAT_supportsOverscan(void);

// Overlay
SDL_Surface* PLAT_initOverlay(void);
void PLAT_quitOverlay(void);
void PLAT_enableOverlay(int enable);

// Power/Hardware
void PLAT_getBatteryStatus(int* is_charging, int* charge);
void PLAT_enableBacklight(int enable);
void PLAT_powerOff(void);
void PLAT_setCPUSpeed(int speed);
void PLAT_setRumble(int strength);
int PLAT_pickSampleRate(int requested, int max);
char* PLAT_getModel(void);
int PLAT_isOnline(void);
int PLAT_setDateTime(int y, int m, int d, int h, int i, int s);
```

**Hardware Platform Features**:
- ✅ Platform-specific input device handling (event0-3, GPIOs, etc.)
- ✅ Hardware framebuffer access (/dev/fb0)
- ✅ DMA/ION buffer management
- ✅ Display engine integration
- ✅ Hardware scaling/sharpening
- ✅ Power management (battery, sleep, backlight)
- ✅ Phase 2 config system integration
- ✅ Platform-specific settings (CPU speeds, audio latency)

---

## 2. Requirements Analysis

### 2.1 Testing Goals

**Primary Objectives**:
1. **Manual UI Testing**: Exercise minui launcher with keyboard/gamepad
2. **Visual Verification**: See rendering output in real-time
3. **Config Testing**: Test all 16 integrated config options interactively
4. **Input Testing**: Test button_swap, analog_sensitivity, etc.
5. **Path Testing**: Verify rom_path, bios_path, saves_path work correctly
6. **Cross-Platform**: Run on Linux (containers), macOS, potentially Windows

**Secondary Objectives**:
1. **Automation**: Screenshot capture, scripted input sequences
2. **CI/CD Integration**: Run automated tests in CI pipeline
3. **Regression Testing**: Catch visual/behavioral regressions
4. **Performance Testing**: FPS, frame timing, input latency

### 2.2 Configurable Platform Requirements

**User Story**: As a developer, I want to test MinUI with different hardware configurations without owning multiple devices.

**Configuration Options Needed**:

| Category | Options | Purpose |
|----------|---------|---------|
| **Display** | Screen size (320x240, 640x480, 720x480, 800x480, 1280x720, etc.) | Test different device resolutions |
| | Pixel format (RGB565, RGB888, RGBA8888) | Test rendering on different color depths |
| | Rotation (0°, 90°, 180°, 270°) | Test portrait/landscape modes |
| **Input** | Button layout (A/B/X/Y positions) | Test different controller mappings |
| | Analog sticks (0, 1, 2 sticks) | Test devices with/without analog |
| | D-pad type (physical, analog-emulated) | Test different input methods |
| | Trigger buttons (L1/R1, L2/R2, L3/R3) | Test full/minimal button sets |
| **Hardware** | Battery simulation (charging, levels) | Test power display |
| | WiFi toggle | Test online features |
| | SD card path | Test custom ROM/BIOS paths |
| **Performance** | Vsync control | Test frame pacing |
| | CPU throttling simulation | Test low-power scenarios |
| | Audio latency | Test audio sync |

### 2.3 Development Workflow Requirements

**Desired Workflow**:
```bash
# Quick iteration cycle
1. Edit code in workspace/all/minui/minui.c
2. Run: ./build_devplatform.sh
3. Test change immediately in SDL window
4. Repeat
```

**vs. Current Hardware Workflow**:
```bash
1. Edit code
2. Cross-compile for device (rg35xx, miyoomini, etc.)
3. Copy SD card image to device
4. Boot device
5. Test
6. Repeat (5-10 minute cycle)
```

**Automation Requirements**:
- Scripted input sequences (JSON/YAML)
- Screenshot capture at key points
- Automated comparison (visual regression)
- Headless mode for CI (Xvfb, offscreen rendering)

---

## 3. Architecture Design

### 3.1 Proposed Platform: "devplatform"

**Name**: `workspace/devplatform/` (or `workspace/sdl/`)

**Design Philosophy**:
- Based on existing MacOS platform (proven SDL2 approach)
- Enhanced with full configurability
- Linux-first (container/QEMU friendly)
- Cross-platform compatibility (Linux, macOS, Windows via SDL2)
- Integration with Phase 2 config system

### 3.2 Core Components

```
workspace/devplatform/
├── platform/
│   ├── platform.c          # Full PLAT_* implementation (SDL2-based)
│   ├── platform.h          # Platform defines (configurable)
│   ├── config_platform.h   # Platform configuration structure
│   └── msettings.h         # Stub for settings API
├── configs/                # Pre-configured platform profiles
│   ├── rg35xx.conf        # 640x480, specific button layout
│   ├── miyoomini.conf     # 640x480, different buttons
│   ├── rgb30.conf         # 720x720, square display
│   ├── generic.conf       # Sensible defaults
│   └── ci.conf            # Headless for CI
├── tools/
│   ├── input_recorder.py  # Record input sequences
│   ├── input_player.py    # Playback recorded inputs
│   └── screenshot.py      # Automated screenshot capture
├── Makefile               # Build minui + minarch for devplatform
├── build.sh               # Quick build script
├── run.sh                 # Launch with config selection
└── README.md              # Usage documentation
```

### 3.3 Configuration System

**Platform Configuration File** (e.g., `configs/generic.conf`):

```ini
[display]
width = 640
height = 480
pixel_format = RGB565  # RGB565, RGB888, RGBA8888
rotation = 0           # 0, 90, 180, 270
scale_mode = integer   # native, aspect, integer, fullscreen

[input]
keyboard_mapping = qwerty  # qwerty, azerty, custom
enable_joystick = true
analog_sticks = 1          # 0, 1, 2
button_layout = nintendo   # nintendo, xbox, playstation

[hardware]
battery_simulate = true
battery_charging = false
battery_level = 75
wifi_enabled = true
rumble_enabled = false

[paths]
sdcard = ./FAKESD
userdata = ./FAKESD/.userdata
saves = ./FAKESD/Saves
roms = ./FAKESD/Roms
bios = ./FAKESD/Bios

[performance]
vsync = true
target_fps = 60
audio_latency = 64
cpu_speed_hint = normal

[debug]
show_fps_overlay = false
log_input_events = false
screenshot_key = F12
quit_key = ESC
```

**Loading Priority**:
1. `devplatform.conf` (in current directory)
2. `~/.config/minui-devplatform.conf` (user home)
3. `configs/generic.conf` (fallback defaults)

### 3.4 SDL2 Architecture

**Video Pipeline**:
```
MinUI Rendering
    ↓
PLAT_flip() / PLAT_blitRenderer()
    ↓
SDL2 Texture (streaming)
    ↓
SDL2 Renderer (hardware accelerated)
    ↓
SDL2 Window (on-screen) or Offscreen (CI)
```

**Input Pipeline**:
```
SDL2 Events (keyboard, joystick, gamepad)
    ↓
Platform Input Mapping (configurable)
    ↓
PLAT_pollInput() → PAD_poll()
    ↓
MinUI/MinArch (button state)
    ↓
Config System (button_swap, analog_sensitivity)
```

### 3.5 Platform Implementation Strategy

**Approach**: Extend MacOS platform with configurability

**Key Changes from MacOS Platform**:

1. **Configuration Loading** (PLAT_initVideo):
```c
SDL_Surface* PLAT_initVideo(void) {
    // Load platform configuration
    platform_config_t* plat_config = load_platform_config();

    // Load Phase 2 MinUI configuration
    #ifdef USE_CONFIG_SYSTEM
    minui_config_t* config = config_load(NULL);
    CONFIG_set(config);
    #endif

    // Create window with configured size
    vid.window = SDL_CreateWindow(
        "MinUI DevPlatform",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        plat_config->display.width,
        plat_config->display.height,
        SDL_WINDOW_SHOWN | (plat_config->display.resizable ? SDL_WINDOW_RESIZABLE : 0)
    );

    // Configure renderer based on vsync setting
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (plat_config->performance.vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    vid.renderer = SDL_CreateRenderer(vid.window, -1, renderer_flags);

    // ... rest of initialization
}
```

2. **Configurable Input Mapping** (PLAT_pollInput):
```c
void PLAT_pollInput(void) {
    pad.just_pressed = BTN_NONE;
    pad.just_released = BTN_NONE;
    pad.just_repeated = BTN_NONE;

    // Update repeat state
    uint32_t tick = SDL_GetTicks();
    for (int i=0; i<BTN_ID_COUNT; i++) {
        int btn = 1 << i;
        if ((pad.is_pressed & btn) && (tick>=pad.repeat_at[i])) {
            pad.just_repeated |= btn;
            pad.repeat_at[i] += PAD_REPEAT_INTERVAL;
        }
    }

    // Poll SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                int btn = map_keyboard_to_button(event.key.keysym.scancode);
                if (btn != BTN_NONE) {
                    PAD_setButton(btn, event.type == SDL_KEYDOWN);
                }
                break;
            }
            case SDL_JOYAXISMOTION: {
                // Handle analog sticks with configured sensitivity
                if (platform_config.input.analog_sticks > 0) {
                    handle_analog_axis(&event.jaxis);
                }
                break;
            }
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP: {
                int btn = map_joy_button_to_button(event.jbutton.button);
                if (btn != BTN_NONE) {
                    PAD_setButton(btn, event.type == SDL_JOYBUTTONDOWN);
                }
                break;
            }
        }
    }
}
```

3. **Simulated Hardware Features**:
```c
void PLAT_getBatteryStatus(int* is_charging, int* charge) {
    if (platform_config.hardware.battery_simulate) {
        *is_charging = platform_config.hardware.battery_charging;
        *charge = platform_config.hardware.battery_level;
    } else {
        *is_charging = 1;
        *charge = 100;
    }
}
```

---

## 4. Implementation Options

### 4.1 Option A: Native SDL2 Platform (Recommended)

**Description**: Build directly on host OS using SDL2

**Pros**:
- ✅ Fastest iteration (no VM/container overhead)
- ✅ Full GPU acceleration
- ✅ Easy debugging (native gdb, lldb)
- ✅ Works on Linux, macOS, Windows
- ✅ Simple setup (apt install libsdl2-dev)

**Cons**:
- ❌ Requires SDL2 installed on host
- ❌ Platform-specific build differences
- ❌ Less isolated from host environment

**Setup**:
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

# macOS
brew install sdl2 sdl2_image sdl2_ttf

# Build
cd workspace/devplatform
make minui
./minui

# Or combined:
./run.sh --config configs/rg35xx.conf
```

**Use Case**: Primary development platform, fastest iteration

---

### 4.2 Option B: Docker Container

**Description**: Containerized X11/Wayland forwarding with SDL2

**Pros**:
- ✅ Consistent environment across developers
- ✅ Easy CI/CD integration
- ✅ Isolated dependencies
- ✅ Reproducible builds

**Cons**:
- ❌ X11 forwarding overhead (slight latency)
- ❌ Limited GPU acceleration (depends on driver passthrough)
- ❌ More complex setup
- ❌ macOS/Windows requires XQuartz/VcXsrv

**Dockerfile**:
```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /minui
COPY . .

# Build devplatform
RUN cd workspace/devplatform && make

# Run with X11 forwarding
ENV DISPLAY=:0
CMD ["workspace/devplatform/minui"]
```

**Usage**:
```bash
# Build container
docker build -t minui-devplatform .

# Run with X11 forwarding (Linux)
docker run -it \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $(pwd)/FAKESD:/minui/workspace/devplatform/FAKESD \
    minui-devplatform

# Run with VNC (headless)
docker run -p 5900:5900 minui-devplatform-vnc
```

**Use Case**: CI/CD, consistent team environment, headless testing

---

### 4.3 Option C: QEMU VM with GPU Passthrough

**Description**: Full Linux VM with virtio-gpu

**Pros**:
- ✅ Most realistic (actual Linux kernel)
- ✅ Can test device-specific kernel modules (if needed)
- ✅ Complete isolation
- ✅ Snapshot/restore support

**Cons**:
- ❌ Slowest iteration (VM boot time)
- ❌ Complex GPU passthrough setup
- ❌ Most resource-intensive
- ❌ Overkill for MinUI (doesn't need kernel features)

**Not Recommended**: MinUI doesn't require kernel-level features, SDL2 is sufficient

---

### 4.4 Option D: Hybrid Approach (Recommended for Teams)

**Description**: Native for development, Docker for CI/CD

**Development Workflow**:
```bash
# Developer workstation (native SDL2)
cd workspace/devplatform
./run.sh --config configs/generic.conf
# Fast iteration, full GPU acceleration
```

**CI/CD Workflow**:
```yaml
# .github/workflows/test-ui.yml
name: UI Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build devplatform
        run: |
          cd workspace/devplatform
          make
      - name: Run automated tests
        run: |
          Xvfb :99 -screen 0 640x480x24 &
          export DISPLAY=:99
          cd workspace/devplatform
          python3 tools/run_tests.py --headless
      - name: Upload screenshots
        uses: actions/upload-artifact@v2
        with:
          name: test-screenshots
          path: workspace/devplatform/screenshots/
```

**Use Case**: Best of both worlds - speed for developers, consistency for CI

---

## 5. Detailed Implementation Plan

### 5.1 Phase 3.1: Foundation (Week 1)

**Goal**: Basic SDL2 platform that can run minui

**Tasks**:
1. ✅ Research complete (this document)
2. Create `workspace/devplatform/` structure
3. Copy MacOS platform as starting point
4. Implement basic platform configuration loading
5. Update Makefile for Linux (remove Homebrew paths)
6. Get minui launcher running with keyboard input
7. Test with existing FAKESD structure

**Deliverable**: `./workspace/devplatform/minui` runs and shows ROM list

**Success Criteria**:
- [ ] minui executable compiles on Linux
- [ ] SDL2 window opens with ROM browser
- [ ] Can navigate with keyboard (arrow keys, Enter, Escape)
- [ ] ROMs load and display names correctly

---

### 5.2 Phase 3.2: Configurability (Week 2)

**Goal**: Platform configuration system

**Tasks**:
1. Design platform config file format (INI or JSON)
2. Implement config parser (config_platform.c)
3. Add config profiles (rg35xx, miyoomini, rgb30, generic)
4. Make screen size configurable
5. Make input mapping configurable
6. Add `run.sh` launcher script with --config option
7. Document configuration options

**Deliverable**: Can switch between device profiles easily

**Success Criteria**:
- [ ] `./run.sh --config configs/rg35xx.conf` uses 640x480
- [ ] `./run.sh --config configs/rgb30.conf` uses 720x720
- [ ] Input mappings change per config
- [ ] README documents all config options

---

### 5.3 Phase 3.3: Input System (Week 3)

**Goal**: Complete input handling (keyboard, joystick, gamepad)

**Tasks**:
1. Implement full keyboard mapping system
2. Add SDL2 joystick/gamepad support (SDL_GameController API)
3. Test button_swap config integration
4. Test analog_sensitivity config integration
5. Add input event logging (for debugging)
6. Create input recorder tool (record sequences)

**Deliverable**: Full input system with recording capability

**Success Criteria**:
- [ ] Keyboard controls work (arrows, WASD, etc.)
- [ ] Xbox/PS4 controller recognized and mapped
- [ ] button_swap config actually swaps A/B
- [ ] analog_sensitivity adjusts stick deadzone
- [ ] Can record and replay input sequences

---

### 5.4 Phase 3.4: Testing Tools (Week 4)

**Goal**: Automation tools for regression testing

**Tasks**:
1. Implement screenshot capture (F12 key)
2. Create input playback tool (Python)
3. Add visual comparison tool (image diff)
4. Create test suite framework
5. Write example test cases
6. Add headless mode (Xvfb/offscreen SDL)
7. CI/CD integration (GitHub Actions)

**Deliverable**: Automated test suite

**Success Criteria**:
- [ ] Can capture screenshots programmatically
- [ ] Can replay recorded input sequences
- [ ] Visual regression tests run in CI
- [ ] Test report shows pass/fail with screenshots

---

### 5.5 Phase 3.5: MinArch Support (Week 5)

**Goal**: Test minarch (emulator frontend)

**Tasks**:
1. Build minarch for devplatform
2. Test ROM loading
3. Test in-game menu
4. Test save states
5. Test config options (display_scale, sharpness, etc.)
6. Document emulator testing workflow

**Deliverable**: Full minarch testing capability

**Success Criteria**:
- [ ] minarch compiles and runs
- [ ] Can load and run a test ROM
- [ ] In-game menu works (save states, options)
- [ ] All 16 config options testable

---

### 5.6 Phase 3.6: Documentation & Polish (Week 6)

**Goal**: Production-ready platform

**Tasks**:
1. Complete README with examples
2. Add troubleshooting guide
3. Create video tutorial
4. Performance optimization
5. Docker image for CI/CD
6. Team onboarding documentation

**Deliverable**: Complete testing platform with docs

**Success Criteria**:
- [ ] New developer can set up in < 15 minutes
- [ ] All features documented with examples
- [ ] CI/CD pipeline running tests automatically
- [ ] Team actively using for development

---

## 6. Resource Requirements

### 6.1 Time Estimate

**Total**: 6 weeks (assuming single developer, part-time)

| Phase | Duration | Complexity |
|-------|----------|------------|
| 3.1 Foundation | 1 week | Medium |
| 3.2 Configurability | 1 week | Medium |
| 3.3 Input System | 1 week | High |
| 3.4 Testing Tools | 1 week | High |
| 3.5 MinArch | 1 week | Medium |
| 3.6 Docs & Polish | 1 week | Low |

**Critical Path**: Phases 3.1 → 3.2 → 3.3 are sequential

**Parallelizable**: Phase 3.4 (tools) can overlap with 3.3 (input)

### 6.2 Dependencies

**Software**:
- SDL2 (libsdl2-dev) - video/audio/input
- SDL2_image (libsdl2-image-dev) - image loading
- SDL2_ttf (libsdl2-ttf-dev) - fonts
- Python 3.8+ - automation tools
- Pillow (Python) - image comparison
- Docker (optional) - containerization
- Xvfb (optional) - headless testing

**Hardware**:
- Development machine with GPU
- USB game controller (for testing)
- 2GB+ RAM
- 1GB disk space

### 6.3 Skills Required

- ✅ C programming (platform.c implementation)
- ✅ SDL2 API knowledge (events, rendering)
- ✅ Python scripting (automation tools)
- ⚠️ Docker basics (if using containers)
- ⚠️ CI/CD configuration (GitHub Actions)

---

## 7. Risk Analysis

### 7.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| SDL2 portability issues | Low | Medium | Test on Linux/macOS/Windows early |
| Input mapping complexity | Medium | High | Use SDL_GameController API (standardized) |
| Performance overhead | Low | Low | SDL2 is hardware-accelerated |
| Config system conflicts | Low | Medium | Use separate namespace (platform_config vs minui_config) |
| CI/CD headless issues | Medium | Medium | Test Xvfb early, have fallback to VNC |

### 7.2 Project Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Scope creep | Medium | High | Stick to 6-phase plan, defer nice-to-haves |
| Time estimation | Medium | Medium | Start with Phase 3.1, re-estimate after |
| Team adoption | Low | High | Prioritize documentation and ease of setup |
| Maintenance burden | Low | Medium | Keep it simple, minimize custom code |

---

## 8. Alternatives Considered

### 8.1 Why Not Extend Test Platform?

**Rejected**: Test platform is specifically designed for unit testing, not runtime/UI testing

**Reasoning**:
- Test platform has no visual output
- No input system
- Would require complete rewrite anyway

**Decision**: Keep test platform for unit tests, create separate devplatform

---

### 8.2 Why Not Use RetroArch Directly?

**Rejected**: MinUI is a custom frontend, not RetroArch

**Reasoning**:
- MinUI has its own UI (minui launcher)
- MinArch is a minimal libretro frontend (different from RetroArch)
- Testing MinUI-specific features requires MinUI platform

**Decision**: Create MinUI-specific testing platform

---

### 8.3 Why Not QEMU Full System Emulation?

**Rejected**: Overkill for SDL2 application

**Reasoning**:
- MinUI doesn't need kernel features
- SDL2 works fine on host OS
- QEMU adds unnecessary complexity/overhead
- Slower iteration cycles

**Decision**: Native SDL2 + optional Docker for CI

---

## 9. Success Metrics

### 9.1 Development Efficiency

**Before Phase 3**:
- Edit → Cross-compile → Flash SD → Boot device → Test: **10+ minutes**
- Visual debugging: Requires physical device
- Config testing: Manual on device

**After Phase 3** (Target):
- Edit → Compile → Run → Test: **< 30 seconds**
- Visual debugging: On-screen immediately
- Config testing: Edit config file, restart

**Target**: **20x faster iteration cycles**

### 9.2 Testing Coverage

**Before Phase 3**:
- Unit tests: Config parsing only (81 tests)
- UI tests: Manual on device
- Regression tests: None

**After Phase 3** (Target):
- Unit tests: 81 (existing)
- Integration tests: 50+ (new)
- UI regression tests: Automated
- Config tests: All 16 options testable

**Target**: **3x increase in test coverage**

### 9.3 Team Adoption

**Target Metrics**:
- [ ] 100% of team members can run devplatform
- [ ] 80% of development done on devplatform (vs device)
- [ ] 50% reduction in device-specific bugs
- [ ] CI/CD catches regressions before merge

---

## 10. Next Steps

### 10.1 Immediate Actions

**For Review** (This Document):
1. [ ] Review architecture design
2. [ ] Approve implementation approach (Option A + D)
3. [ ] Confirm 6-phase timeline acceptable
4. [ ] Identify any missing requirements

**After Approval**:
1. [ ] Create `workspace/devplatform/` directory
2. [ ] Begin Phase 3.1 (Foundation)
3. [ ] Set up project tracking (GitHub issues/milestones)

### 10.2 Decision Points

**Need User Input On**:
1. Platform name: `devplatform` vs `sdl` vs `desktop` vs other?
2. Config format: INI vs JSON vs YAML?
3. Priority: Speed (native) vs Consistency (Docker)?
4. Timeline: Acceptable to spend 6 weeks on this?

### 10.3 Open Questions

1. Should we support Windows? (SDL2 works, but untested)
2. Should we support Android? (SDL2 works, different use case)
3. Should we integrate with VSCode debugger configuration?
4. Should we create Nix/Flake for reproducible environment?

---

## 11. Conclusion

### Summary

Phase 3 proposes creating a **full-featured, configurable development platform** based on SDL2 that enables:
- ✅ **Fast iteration**: < 30 second edit-compile-test cycles
- ✅ **Visual testing**: See MinUI rendering in real-time
- ✅ **Automated testing**: CI/CD with screenshot comparison
- ✅ **Config testing**: Test all 16 integrated options interactively
- ✅ **Cross-platform**: Linux (primary), macOS, potentially Windows

### Recommendation

**Implement Option D (Hybrid Approach)**:
- Native SDL2 for development (fastest iteration)
- Docker container for CI/CD (consistency)
- 6-phase rollout over 6 weeks
- Start with Phase 3.1 (Foundation) immediately

### Expected Benefits

1. **20x faster development cycles** (10 minutes → 30 seconds)
2. **3x increase in test coverage** (unit + integration + UI tests)
3. **Reduced hardware dependency** (develop without devices)
4. **Better onboarding** (new developers productive in < 15 minutes)
5. **Regression prevention** (automated visual testing in CI)

---

**Document Status**: Ready for Review
**Author**: Claude (AI Assistant)
**Date**: 2025-10-29
**Version**: 1.0

---

## Appendix A: Comparison Matrix

| Feature | Test Platform | MacOS Platform | DevPlatform (Proposed) |
|---------|---------------|----------------|------------------------|
| **Purpose** | Unit testing | Compile checking | Full dev/test |
| **Runs minui** | ❌ No | ✅ Yes | ✅ Yes |
| **Runs minarch** | ❌ No | ⚠️ Partial | ✅ Yes |
| **Visual output** | ❌ No | ✅ Yes | ✅ Yes |
| **Input simulation** | ❌ No | ⚠️ Keyboard only | ✅ Keyboard + Gamepad |
| **Configurable** | ❌ No | ❌ No | ✅ Yes |
| **Screen size** | N/A | Fixed 640x480 | Configurable |
| **Phase 2 config** | ✅ Yes (parsing) | ❌ No | ✅ Yes (full) |
| **Automation** | ✅ Yes (unit tests) | ❌ No | ✅ Yes (UI tests) |
| **CI/CD ready** | ✅ Yes | ❌ No | ✅ Yes |
| **Cross-platform** | ✅ Linux | ⚠️ macOS only | ✅ Linux/macOS/Windows |
| **Setup time** | < 5 min | ~10 min | < 15 min |
| **Maintenance** | Low | Low | Medium |

---

## Appendix B: Example Test Case

**Test**: Verify button_swap configuration works

**Input Sequence** (JSON):
```json
{
  "test_name": "button_swap_test",
  "config": {
    "button_swap": 1
  },
  "steps": [
    {"action": "wait", "duration": 1000},
    {"action": "key", "key": "Return", "comment": "Press A (Return key)"},
    {"action": "screenshot", "name": "before_swap"},
    {"action": "wait", "duration": 500},
    {"action": "key", "key": "Return", "comment": "Should act as B when swapped"},
    {"action": "screenshot", "name": "after_swap"},
    {"action": "compare", "expected": "after_swap_expected.png", "threshold": 0.95}
  ]
}
```

**Expected**: When button_swap=1, pressing "Return" (normally A) should act as B

**Validation**: Visual comparison of screenshots

---

## Appendix C: Build Commands Reference

**Current Test Platform**:
```bash
cd workspace/test
make clean && make test
# Runs 81 unit tests for config system
```

**Current MacOS Platform**:
```bash
cd workspace/all/minui
mkdir -p build/macos
gcc minui.c -o build/macos/minui \
  -I. -I../common/ -I../../macos/platform/ \
  -I/opt/homebrew/include -L/opt/homebrew/lib \
  ../common/scaler.c ../common/utils.c ../common/api.c \
  ../../macos/platform/platform.c \
  -DPLATFORM=\"macos\" -DUSE_SDL2 -Ofast -std=gnu99 \
  -ldl -flto -lSDL2 -lSDL2_image -lSDL2_ttf \
  -lpthread -lm -lz -fsanitize=address
build/macos/minui
```

**Proposed DevPlatform**:
```bash
cd workspace/devplatform
make                         # Build minui + minarch
./run.sh                     # Run with default config
./run.sh --config rg35xx     # Run emulating rg35xx
make test                    # Run automated test suite
```

---

**End of Document**
