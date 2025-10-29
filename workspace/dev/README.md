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

### Known Limitations

- No joystick/gamepad support yet (Phase 3.3)
- Button repeat logic not implemented
- Scalers return NULL (SDL handles scaling)

### Next Steps (Future Phases)

- **Phase 3.3**: Joystick/gamepad support
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
