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

### Next Steps (Future Phases)

- **Phase 3.4**: Testing tools (automation, screenshot capture)
- **Phase 3.5**: Core emulator support

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
