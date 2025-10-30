# MinUI Dev Platform

SDL2-based development platform for MinUI, enabling native development and testing without hardware.

## Phase 3.1 - Foundation (COMPLETE)

### Features Implemented

- ✅ Full SDL2 video system (640x480, configurable)
- ✅ Keyboard input mapping (arrow keys, ZXAS for buttons)
- ✅ Basic audio support
- ✅ Configuration system integration
- ✅ msettings stubs
- ✅ Successfully builds minui.elf and minarch.elf

### Building

```bash
export CROSS_COMPILE=" "
make PLATFORM=dev
```

### Keyboard Controls

- **D-Pad**: Arrow keys
- **A Button**: X key
- **B Button**: Z key
- **X Button**: S key
- **Y Button**: A key
- **L1/R1**: Q/W keys
- **L2/R2**: E/R keys
- **Start**: Enter
- **Select**: Right Shift
- **Menu**: Escape
- **Power**: P key

### Directory Structure

- `platform/` - Platform implementation files
  - `platform.h` - Button mappings and constants
  - `platform.c` - SDL2 PLAT_* implementations
  - `msettings.h` - Stub settings header
  - `makefile.env` - Build configuration
  - `phase2_flags.mk` - Feature flags
- `libmsettings/` - Stub library
- `keymon/` - Stub (not needed)
- `cores/` - Stub (Phase 3.5)

### Build Outputs

- `../all/minui/build/dev/minui.elf` - MinUI launcher
- `../all/minarch/build/dev/minarch.elf` - MinArch emulator frontend

## Phase 3.2 - Configurability (COMPLETE)

### Features Implemented

- ✅ INI-based platform configuration (`platform.conf`)
- ✅ Device profiles for real hardware simulation
  - RG35XX (640x480, 16-bit)
  - RGB30 (720x720, 16-bit)
  - Miyoo Mini (640x480, 16-bit)
  - TrimUI Smart (320x240, 16-bit)
  - Custom profile support
- ✅ Configurable window size and pixel format
- ✅ Windowed/fullscreen mode support
- ✅ Dynamic video surface resizing
- ✅ VSync toggle

### Configuration

Edit `workspace/platform.conf` to change settings:

```ini
[general]
profile = rg35xx  # or rgb30, miyoomini, trimuismart, custom
window_mode = windowed  # or fullscreen
vsync = 1
pixel_format = RGB565  # or ARGB8888
```

Each profile accurately simulates real device specs.

### Testing Different Devices

```bash
# Test RG35XX (640x480)
# Edit platform.conf: profile = rg35xx
export CROSS_COMPILE=" " && make PLATFORM=dev

# Test RGB30 (720x720 square screen)
# Edit platform.conf: profile = rgb30
export CROSS_COMPILE=" " && make PLATFORM=dev
```

## Phase 3.3 - Joystick/Gamepad Support (COMPLETE)

### Features Implemented

- ✅ SDL Game Controller API integration
- ✅ Automatic gamepad detection and enumeration
- ✅ Standard button mapping (Xbox/PlayStation/Switch layout)
- ✅ Analog stick support (left stick as D-pad)
- ✅ Analog trigger support (L2/R2)
- ✅ Hotplug support (connect/disconnect during runtime)
- ✅ Fallback to SDL Joystick API for unknown controllers
- ✅ Keyboard + gamepad simultaneous input

### Controller Support

**Tested Controllers:**
- Xbox Controllers (360, One, Series X/S)
- PlayStation Controllers (DualShock 4, DualSense)
- Nintendo Switch Pro Controller
- Generic USB/Bluetooth gamepads

**Button Mapping (Xbox layout):**
- D-pad → D-pad
- A (bottom) → B button
- B (right) → A button
- X (left) → Y button
- Y (top) → X button
- LB/RB → L1/R1
- LT/RT → L2/R2 (analog triggers, >50% = pressed)
- L3/R3 → L3/R3 (stick clicks)
- Back/Select → Select
- Start → Start
- Guide/Home → Menu

**Analog Sticks:**
- Left stick → D-pad navigation (50% deadzone)
- Right stick → Not mapped (reserved for future use)

### Using Gamepad

1. Connect your controller before or after starting
2. The platform auto-detects and configures it
3. Hotplug supported - connect/disconnect anytime
4. Keyboard always works as fallback

**Console output:**
```
dev platform: Found 1 joystick(s)
dev platform: Opened gamepad: Xbox Series X Controller
```

### Known Limitations

- Button repeat logic not implemented
- Scalers return NULL (SDL handles scaling)
- Right analog stick not mapped yet

## Phase 3.4 - Testing Tools (COMPLETE)

### Features Implemented

- ✅ Screenshot capture (F12 key)
- ✅ Input recording (F11 key)
- ✅ Headless mode for CI/CD
- ✅ Python automation tools
- ✅ Visual regression testing
- ✅ Test suite framework
- ✅ CI/CD integration (GitHub Actions)

### Keyboard Shortcuts

**Testing Keys:**
- **F12**: Capture screenshot → `./screenshots/screenshot_YYYYMMDD_HHMMSS.png`
- **F11**: Start/stop input recording → `./input_recording_YYYYMMDD_HHMMSS.csv`

### Testing Tools

Located in `tools/` directory:

1. **run_tests.py** - Automated test suite runner
   ```bash
   cd tools
   ./run_tests.py
   ./run_tests.py --headless  # For CI
   ```

2. **input_player.py** - Replay recorded input sequences
   ```bash
   ./input_player.py recording.csv
   ./input_player.py recording.csv --speed 2.0
   ```

3. **compare_images.py** - Visual regression testing
   ```bash
   ./compare_images.py expected.png actual.png
   ./compare_images.py expected.png actual.png --threshold 0.95 --output diff.png
   ```

See `tools/README.md` for detailed documentation.

### Test Suite

Located in `tests/` directory:
- Test cases defined in JSON format
- Example: `test_basic_navigation.json`
- Expected screenshots in `tests/expected/`
- Input recordings in `tests/recordings/`

See `tests/README.md` for test authoring guide.

### Environment Variables

**MINUI_HEADLESS=1** - Run in headless mode (no window)
```bash
MINUI_HEADLESS=1 ./minui.elf
```

**MINUI_SCREENSHOTS_DIR** - Custom screenshots directory
```bash
MINUI_SCREENSHOTS_DIR=/tmp/shots ./minui.elf
```

### CI/CD Integration

Automated testing runs on every push via GitHub Actions:
- `.github/workflows/test-dev-platform.yml`
- Runs tests in headless mode with Xvfb
- Uploads test reports and screenshots as artifacts
- Posts results to pull requests

### Usage Examples

**Manual Testing:**
```bash
# Build
export CROSS_COMPILE=" " && make PLATFORM=dev

# Run and test manually
cd ../all/minui/build/dev
./minui.elf

# Press F12 to capture screenshots
# Press F11 to record inputs
```

**Automated Testing:**
```bash
# Run test suite
cd tools
./run_tests.py

# View results
open ../test_output/test_report.html
```

**Headless Testing (CI):**
```bash
# Start virtual display
Xvfb :99 -screen 0 640x480x24 &
export DISPLAY=:99

# Run tests
MINUI_HEADLESS=1 ./run_tests.py --headless
```

### Dependencies

**Runtime:**
- SDL2
- SDL2_image (for PNG screenshot capture)
- SDL2_ttf

**Testing Tools:**
- Python 3.8+
- python3-xlib (for input playback)
- Pillow (for image comparison)

**Install dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev \
                     python3 python3-pip python3-xlib
pip3 install Pillow

# macOS
brew install sdl2 sdl2_image sdl2_ttf python3
pip3 install Pillow
```

## Phase 3.5 - MinArch Emulator Support (COMPLETE)

### Features Implemented

- ✅ MinArch frontend builds for dev platform
- ✅ Libretro core build system (native x86_64/arm64)
- ✅ Gambatte core (Game Boy/Game Boy Color emulator)
- ✅ Core loading and API validation
- ✅ Test ROM directory structure
- ✅ Unit tests for core loading
- ✅ Integration test scripts
- ✅ Comprehensive testing guide

### Building MinArch

```bash
export CROSS_COMPILE=" "
cd workspace/dev
make PLATFORM=dev  # Builds both minui and minarch
```

### Building Cores

```bash
cd workspace/dev/cores

# Build gambatte (Game Boy/GBC)
PLATFORM=dev make gambatte

# Built cores appear in:
# workspace/dev/cores/output/gambatte_libretro.so
```

### Testing MinArch

**Unit tests** (verify core loading):
```bash
cd workspace/dev/tests
make run
```

**Integration test** (with ROM):
```bash
cd workspace/dev/tests

# Requires a Game Boy ROM at /tmp/minui_dev/Roms/GB/test.gb
./test_minarch.sh

# Or specify ROM path:
./test_minarch.sh /path/to/your/rom.gb
```

### Test ROM Structure

```
/tmp/minui_dev/
├── Roms/
│   └── GB/              # Place Game Boy ROMs here
│       └── README.txt   # Lists recommended homebrew ROMs
├── Bios/
│   └── GB/              # BIOS files (not needed for GB)
├── Saves/
│   └── GB/              # Save files (auto-created)
└── .userdata/
    └── dev/
        └── GB-gambatte/ # Save states and config
```

### Supported Cores

| Core | System | Status | Extensions |
|------|--------|--------|------------|
| **gambatte** | Game Boy / Game Boy Color | ✅ Working | .gb, .gbc, .dmg |

**Adding more cores**: Edit `workspace/dev/cores/makefile` and run `PLATFORM=dev make <corename>`

### Architecture Support

The dev platform builds natively for your host architecture:
- **x86_64**: Standard Linux/macOS development
- **arm64**: M1/M2 Mac Docker environment

No cross-compilation needed - cores build for the native architecture automatically.

### Testing Workflow

See **[TESTING_GUIDE.md](./TESTING_GUIDE.md)** for comprehensive testing documentation including:
- Unit test suite
- Manual integration tests
- Recommended test ROMs
- Debugging workflows
- CI/CD integration
- Performance testing

### Keyboard Controls (MinArch)

Same as MinUI, plus:
- **ESC**: In-game menu
- **Fast-forward**: (configurable in menu)

### In-Game Menu Features

- Save states (10 slots)
- Load states
- Reset game
- Display options (scaling, sharpness, filters)
- Audio options
- Fast-forward speed
- Exit to MinUI

### Example Testing Session

```bash
# 1. Build everything
cd workspace/dev
export CROSS_COMPILE=" "
make PLATFORM=dev

# 2. Build gambatte core
cd cores
PLATFORM=dev make gambatte

# 3. Verify with unit tests
cd ../tests
make run

# 4. Download a test ROM (Tobu Tobu Girl - free homebrew)
mkdir -p /tmp/minui_dev/Roms/GB
cd /tmp/minui_dev/Roms/GB
wget https://github.com/SimonLarsen/tobutobugirl/releases/download/v1.0/tobutobugirl.gb

# 5. Test!
cd ~/workspace/dev/tests  # Adjust path as needed
./test_minarch.sh /tmp/minui_dev/Roms/GB/tobutobugirl.gb
```

### Next Steps (Future Phases)

- **Phase 3.6**: Additional cores (fceumm, snes9x2005)
- **Phase 3.7**: Automated visual regression tests
- **Phase 3.8**: Performance benchmarking framework

### Technical Notes

- Uses SDL2 for video, input, and events
- All msettings functions are stubs (implemented in platform.c)
- Build requires `CROSS_COMPILE=" "` environment variable
- PREFIX set to `/tmp/dev_prefix` for stub library
- Configuration integrated via Phase 2 feature flags

### Requirements

- SDL2 development libraries
- SDL2_image
- SDL2_ttf
- gcc
- make

Check SDL2 availability:
```bash
pkg-config --modversion sdl2 SDL2_image SDL2_ttf
```
