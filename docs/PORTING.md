# MinUI Platform Porting Guide

This guide walks through the process of adding support for a new handheld device
to MinUI. It assumes familiarity with C programming, cross-compilation, and
embedded Linux systems.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Platform Structure Overview](#platform-structure-overview)
3. [Step-by-Step Porting Process](#step-by-step-porting-process)
4. [Platform Implementation Details](#platform-implementation-details)
5. [Testing and Validation](#testing-and-validation)
6. [Common Pitfalls](#common-pitfalls)
7. [Example Implementations](#example-implementations)

---

## Prerequisites

Before starting a port, gather the following information about your target device:

### Hardware Specifications
- **CPU**: Architecture (ARM, MIPS, x86), instruction set, frequency
- **RAM**: Total memory available
- **Display**: Resolution, color depth (RGB565, XRGB8888), refresh rate
- **Input**: Button layout, analog sticks, special buttons
- **Audio**: Supported sample rates, buffer sizes
- **Storage**: SD card size/speed constraints
- **Battery**: Monitoring interface (if available)

### Software Stack
- **OS**: Linux kernel version, custom patches
- **Libraries**: SDL version, available system libraries
- **Toolchain**: GCC/Clang version, libc variant (glibc, musl, uclibc)
- **Graphics**: Framebuffer driver (fbdev, DRM/KMS), GPU acceleration
- **Audio**: ALSA, OSS, or other audio system

### Build Environment
- Access to manufacturer SDK or custom toolchain
- Docker support for reproducible builds
- Ability to deploy and test on actual hardware

---

## Platform Structure Overview

Each platform in MinUI follows a standard directory structure:

```
workspace/<platform>/
├── makefile                    # Top-level platform makefile
├── platform/
│   ├── makefile.env           # Compiler flags and toolchain config
│   ├── platform.h             # Platform-specific constants
│   └── platform.c             # Platform API implementations
├── libmsettings/              # Settings persistence library
│   ├── msettings.h
│   └── msettings.c
├── keymon/                    # Background key monitoring (optional)
│   └── keymon.c
├── cores/                     # Platform-specific core builds
│   └── makefile
└── [optional components]
    ├── overclock/             # CPU frequency control
    ├── install/               # Installation scripts
    └── show/                  # Boot splash screen
```

---

## Step-by-Step Porting Process

### Step 1: Create Platform Directory Structure

```bash
cd workspace
mkdir -p mydevice/platform
mkdir -p mydevice/libmsettings
mkdir -p mydevice/cores
```

### Step 2: Define Platform Constants

Create `workspace/mydevice/platform/platform.h`:

```c
#ifndef PLATFORM_H
#define PLATFORM_H

// Display configuration
#define FIXED_WIDTH     640    // Native horizontal resolution
#define FIXED_HEIGHT    480    // Native vertical resolution
#define FIXED_BPP       2      // Bytes per pixel (2=RGB565, 4=XRGB8888)
#define FIXED_SCALE     2      // UI scaling factor (1-3)
#define FIXED_DEPTH     (FIXED_BPP * 8)
#define FIXED_PITCH     (FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE      (FIXED_PITCH * FIXED_HEIGHT)

// Platform identification
#define PLATFORM_NAME   "mydevice"

// Button mapping (example)
#define BTN_ID_A        SDLK_SPACE
#define BTN_ID_B        SDLK_LALT
#define BTN_ID_X        SDLK_LSHIFT
#define BTN_ID_Y        SDLK_LCTRL
// ... define all buttons

// Optional features
#define HAS_ANALOG_STICK        1
#define HAS_BATTERY_MONITORING  1
#define HAS_WIFI                0
#define SUPPORTS_OVERSCAN       0

// Resource paths
#define SDCARD_PATH     "/mnt/sdcard"
#define DATETIME_PATH   "/sys/class/rtc/rtc0/time"

#endif
```

### Step 3: Configure Build Environment

Create `workspace/mydevice/platform/makefile.env`:

```makefile
# Toolchain configuration
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

# Compiler flags
ARCH_FLAGS = -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4
COMMON_FLAGS = $(ARCH_FLAGS) -O3 -ffast-math -fno-common
CFLAGS = $(COMMON_FLAGS) -std=gnu99
CXXFLAGS = $(COMMON_FLAGS) -std=gnu++11
LDFLAGS = -lm -lpthread -lSDL -lSDL_image -lSDL_ttf

# Platform-specific defines
DEFINES = -DPLATFORM_MYDEVICE

# Include paths
INCLUDES = -I$(WORKSPACE)/mydevice/platform \
           -I$(SYSROOT)/usr/include \
           -I$(SYSROOT)/usr/include/SDL

# Library paths
LDFLAGS += -L$(SYSROOT)/usr/lib
```

### Step 4: Implement Platform Abstraction Layer

Create `workspace/mydevice/platform/platform.c` and implement the required functions
from `workspace/all/common/api.h`. See [Platform Implementation Details](#platform-implementation-details)
for comprehensive documentation of each function.

**Minimal required implementations:**

```c
#include <SDL/SDL.h>
#include "platform.h"
#include "api.h"

// === Video Subsystem ===

SDL_Surface* PLAT_initVideo(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface* screen = SDL_SetVideoMode(
        FIXED_WIDTH, FIXED_HEIGHT, FIXED_DEPTH,
        SDL_HWSURFACE | SDL_DOUBLEBUF
    );
    SDL_ShowCursor(0);
    return screen;
}

void PLAT_quitVideo(void) {
    SDL_Quit();
}

void PLAT_clearVideo(SDL_Surface* screen) {
    SDL_FillRect(screen, NULL, 0);
}

void PLAT_clearAll(void) {
    // Clear all video buffers if needed
}

void PLAT_setVsync(int vsync) {
    // Configure vsync if supported
}

// === Input Subsystem ===

void PLAT_initInput(void) {
    SDL_Init(SDL_INIT_JOYSTICK);
    // Initialize device-specific input
}

void PLAT_quitInput(void) {
    // Cleanup input resources
}

void PLAT_pollInput(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Map SDL events to PAD_Context
        // Update pad.is_pressed, pad.just_pressed, etc.
    }
}

int PLAT_shouldWake(void) {
    // Return 1 if device should wake from sleep
    return 0;
}

// === Power Management ===

void PLAT_getBatteryStatus(int* is_charging, int* charge) {
    // Read from /sys or device-specific interface
    // charge: 0, 10, 20, 40, 60, 80, 100
    *is_charging = 0;
    *charge = 100;
}

void PLAT_enableBacklight(int enable) {
    // Control backlight via sysfs or ioctl
}

void PLAT_powerOff(void) {
    // Trigger system shutdown
    system("poweroff");
}

void PLAT_setCPUSpeed(int speed) {
    // Adjust CPU governor/frequency
    switch (speed) {
        case CPU_SPEED_MENU:
            // Set to ~600 MHz
            break;
        case CPU_SPEED_POWERSAVE:
            // Set to ~800 MHz
            break;
        case CPU_SPEED_NORMAL:
            // Set to ~1000 MHz
            break;
        case CPU_SPEED_PERFORMANCE:
            // Set to max MHz
            break;
    }
}

// === Platform Info ===

char* PLAT_getModel(void) {
    return "MyDevice";
}

int PLAT_isOnline(void) {
    // Check network connectivity if applicable
    return 0;
}

int PLAT_setDateTime(int y, int m, int d, int h, int i, int s) {
    // Set system date/time
    return 0;
}

// === Graphics Scaling ===

SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch) {
    // Handle dynamic resolution changes
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, FIXED_DEPTH,
                                0xF800, 0x07E0, 0x001F, 0);
}

void PLAT_setVideoScaleClip(int x, int y, int width, int height) {
    // Set scaling clip region
}

void PLAT_setNearestNeighbor(int enabled) {
    // Toggle nearest-neighbor vs bilinear scaling
}

void PLAT_setSharpness(int sharpness) {
    // Apply sharpness filter (SHARPNESS_SHARP/CRISP/SOFT)
}

scaler_t PLAT_getScaler(GFX_Renderer* renderer) {
    // Return appropriate scaling function
    return scale2x_c16; // Example
}

void PLAT_blitRenderer(GFX_Renderer* renderer) {
    // Blit scaled frame to screen
}

void PLAT_flip(SDL_Surface* screen, int sync) {
    SDL_Flip(screen);
}
```

### Step 5: Implement Settings Library

Create `workspace/mydevice/libmsettings/msettings.c`:

```c
#include "msettings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_PATH "/mnt/sdcard/.userdata/mydevice/settings.txt"

int InitSettings(void) {
    // Create settings directory if needed
    system("mkdir -p /mnt/sdcard/.userdata/mydevice");
    return 0;
}

int GetBrightness(void) {
    // Read from /sys/class/backlight/...
    return 5; // 0-10 scale
}

int SetBrightness(int val) {
    // Write to backlight interface
    return 0;
}

int GetVolume(void) {
    // Read from ALSA mixer or device
    return 8;
}

int SetVolume(int val) {
    // Set volume via amixer or direct interface
    return 0;
}

// Implement other settings functions...
```

### Step 6: Create Platform Makefile

Create `workspace/mydevice/makefile`:

```makefile
include platform/makefile.env

.PHONY: all clean libmsettings

all: libmsettings
	@echo "Building MyDevice platform components"

libmsettings:
	$(MAKE) -C libmsettings

clean:
	$(MAKE) -C libmsettings clean
```

### Step 7: Integrate with Main Build System

Add your platform to `workspace/makefile`:

```makefile
PLATFORMS ?= miyoomini rg35xx mydevice  # Add your platform

...

mydevice:
	$(MAKE) PLATFORM=mydevice -C mydevice
```

And to the root `makefile`:

```makefile
system: ...
	...
	mkdir -p "$(BUILD_PATH)/SYSTEM/mydevice"
	cp -r "$(WORKSPACE_PATH)/mydevice/build/"* "$(BUILD_PATH)/SYSTEM/mydevice/"
```

---

## Platform Implementation Details

### Critical Functions

#### Video Management

**`SDL_Surface* PLAT_initVideo(void)`**
- Initialize SDL video subsystem
- Set display mode to native resolution
- Hide cursor
- Return main screen surface
- Called once at application startup

**`void PLAT_flip(SDL_Surface* screen, int sync)`**
- Present rendered frame to display
- `sync`: if 1, wait for vsync; if 0, return immediately
- Critical path: optimize for minimal latency

**`void PLAT_clearVideo(SDL_Surface* screen)`**
- Fill screen with black (0x0000)
- Called between scene transitions

**`SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch)`**
- Create surface for dynamic resolution
- Used by Minarch when core changes resolution
- Must match pixel format of main screen

#### Scaling and Rendering

**`scaler_t PLAT_getScaler(GFX_Renderer* renderer)`**
- Return function pointer for software scaling
- Choose based on `renderer->scale` and CPU capabilities
- Common options: `scale1x`, `scale2x`, `scale3x`, nearest-neighbor, bilinear

**`void PLAT_blitRenderer(GFX_Renderer* renderer)`**
- Execute the scaling pipeline
- Apply sharpness/effect filters
- Blit to screen surface
- Performance-critical: consider SIMD optimization

#### Input Handling

**`void PLAT_pollInput(void)`**
- Sample all input devices
- Update global `PAD_Context pad` structure
- Set `pad.is_pressed` bits for currently held buttons
- Set `pad.just_pressed` for newly pressed buttons (one frame only)
- Set `pad.just_released` for newly released buttons
- Handle analog stick axes: `pad.laxis.x/y`, `pad.raxis.x/y`
- Called every frame (60 Hz)

**Button Event Logic:**
```c
void PLAT_pollInput(void) {
    static uint32_t prev_pressed = 0;
    uint32_t curr_pressed = 0;

    // Sample hardware (SDL, /dev/input, etc.)
    if (button_a_down) curr_pressed |= (1 << BTN_ID_A);
    if (button_b_down) curr_pressed |= (1 << BTN_ID_B);
    // ... etc

    pad.is_pressed = curr_pressed;
    pad.just_pressed = curr_pressed & ~prev_pressed;
    pad.just_released = prev_pressed & ~curr_pressed;
    prev_pressed = curr_pressed;
}
```

#### Power Management

**`void PLAT_getBatteryStatus(int* is_charging, int* charge)`**
- Read battery level from sysfs, e.g. `/sys/class/power_supply/battery/capacity`
- Quantize charge to: 0, 10, 20, 40, 60, 80, or 100
- Detect charging state from `status` file (Charging/Discharging)
- Called periodically by background thread

**`void PLAT_setCPUSpeed(int speed)`**
- Change CPU governor and frequency
- Example governors: `powersave`, `ondemand`, `performance`
- Write to `/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`
- And `/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed`
- Failure is non-fatal; log and continue

#### Optional Features

**`SDL_Surface* PLAT_initOverlay(void)` / `void PLAT_enableOverlay(int enable)`**
- For devices with dedicated OSD layer
- Display battery/volume icons without disturbing main framebuffer
- Can be no-op if unsupported

**`void PLAT_setRumble(int strength)`**
- Control vibration motor (0-100)
- Write to GPIO or PWM interface
- Fallback to no-op if hardware lacks rumble

**`int PLAT_pickSampleRate(int requested, int max)`**
- Choose audio sample rate supported by hardware
- Common rates: 22050, 32000, 44100, 48000
- Return closest match to `requested` not exceeding `max`

---

## Testing and Validation

### Phase 1: Boot Test
1. Build minimal platform with stubbed functions
2. Deploy to device SD card
3. Verify MinUI launches and displays UI
4. Test input responsiveness
5. Check for crashes or hangs

### Phase 2: Graphics Test
1. Launch a simple core (e.g., gambatte for Game Boy)
2. Verify correct resolution and aspect ratio
3. Test scaling modes: native, aspect, fullscreen
4. Check framerate (should be smooth 60 FPS)
5. Test sharpness settings

### Phase 3: Audio Test
1. Play game with music/sound effects
2. Check for crackling, popping, or stuttering
3. Verify volume control works
4. Test audio at different CPU speeds

### Phase 4: Power Test
1. Check battery indicator accuracy
2. Verify charging state detection
3. Test auto-sleep functionality
4. Ensure clean shutdown on power button

### Phase 5: Compatibility Test
1. Test multiple cores (GBA, SNES, Genesis, PSX)
2. Verify save states work correctly
3. Check in-game menu functionality
4. Test fast-forward and rewind

### Debugging Tips
- Enable debug logging in `api.h`: `#define DEBUG 1`
- Add verbose prints in `PLAT_*` functions
- Use serial console if available
- Check `/var/log/messages` or equivalent
- Test on emulator (QEMU) if feasible before hardware

---

## Common Pitfalls

### 1. Color Format Mismatches
- Ensure FIXED_BPP matches actual framebuffer depth
- RGB565 vs BGR565: check byte order
- Handle alpha channel correctly for XRGB8888

### 2. Input Lag
- `PLAT_pollInput()` called every frame?
- Ensure no blocking I/O in input path
- Check for vsync conflicts

### 3. Screen Tearing
- Implement proper double buffering
- Use SDL_DOUBLEBUF or manual buffer swapping
- Respect vsync settings

### 4. Audio Desync
- Buffer size too large: high latency
- Buffer size too small: underruns
- Sample rate mismatch: pitch shift
- Use `PLAT_pickSampleRate()` correctly

### 5. Performance Issues
- Profile scaling functions: PLAT_blitRenderer() hotspot
- Consider NEON/SIMD optimizations for ARM
- Adjust CPU speed modes for balance
- Check for memory leaks

### 6. Path Hardcoding
- Don't hardcode `/mnt/sdcard` if mount point varies
- Use SDCARD_PATH from platform.h
- Handle missing directories gracefully

---

## Example Implementations

For reference, study these existing platform ports:

### Simple Platforms (Good Starting Points)
- **`workspace/miyoomini/`**: Well-documented, clean SDL implementation
- **`workspace/trimui/`**: Minimal feature set, straightforward

### Advanced Platforms (Reference for Optimization)
- **`workspace/rgb30/`**: Custom framebuffer scaling
- **`workspace/rg35xxplus/`**: Complex input mapping
- **`workspace/m17/`**: Modern DRM/KMS approach

### Key Files to Study
- `workspace/all/common/api.c`: Reference implementations
- `workspace/all/common/scaler.c`: Software scaling functions
- `workspace/all/minarch/minarch.c`: How PAL is used by frontend

---

## Next Steps

After completing the port:

1. **Documentation**: Add device to main README.md
2. **Testing**: Run full compatibility suite
3. **Optimization**: Profile and optimize hotspots
4. **Contribution**: Submit pull request with port
5. **Community**: Share build instructions and feedback

For questions or assistance, refer to:
- [ARCHITECTURE.md](ARCHITECTURE.md) - System design
- [API.md](API.md) - Complete API reference
- GitHub issues and discussions

---

**Good luck with your port! MinUI's architecture makes it straightforward to add
new devices while maintaining quality across the entire platform ecosystem.**
