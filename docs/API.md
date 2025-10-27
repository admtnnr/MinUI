# MinUI Platform API Reference

This document provides comprehensive documentation for the MinUI Platform Abstraction
Layer (PAL) API defined in `workspace/all/common/api.h`. Platform implementers must
provide these functions in their `workspace/<platform>/platform/platform.c` file.

## Table of Contents

1. [Overview](#overview)
2. [Video Subsystem](#video-subsystem)
3. [Audio Subsystem](#audio-subsystem)
4. [Input Subsystem](#input-subsystem)
5. [Power Management](#power-management)
6. [Platform Information](#platform-information)
7. [Graphics Rendering](#graphics-rendering)
8. [Data Types](#data-types)
9. [Constants](#constants)
10. [Helper Macros](#helper-macros)

---

## Overview

The Platform Abstraction Layer provides a unified interface for MinUI and Minarch
to interact with diverse hardware platforms. Each platform implements these functions
according to its specific hardware capabilities and software environment.

### Design Principles

- **Zero overhead**: Abstractions compile to direct hardware access where possible
- **Weak symbols**: Default implementations provided, platforms can override
- **Fail-safe**: Functions should handle errors gracefully and log issues
- **Thread safety**: Document thread-safety requirements for each function

### Implementation Pattern

```c
// In workspace/<platform>/platform/platform.c

#include "api.h"
#include "platform.h"

// Implement each PLAT_* function
// Use FALLBACK_IMPLEMENTATION for optional functions with defaults
```

---

## Video Subsystem

### PLAT_initVideo

```c
SDL_Surface* PLAT_initVideo(void);
```

Initialize the video subsystem and return the main screen surface.

**Returns:**
- Pointer to SDL_Surface representing the screen, or NULL on failure

**Responsibilities:**
- Initialize SDL video subsystem (SDL_Init(SDL_INIT_VIDEO))
- Create video mode matching FIXED_WIDTH × FIXED_HEIGHT × FIXED_DEPTH
- Set appropriate surface flags (SDL_HWSURFACE, SDL_DOUBLEBUF, etc.)
- Hide mouse cursor (SDL_ShowCursor(0))
- Clear screen to black

**Example:**
```c
SDL_Surface* PLAT_initVideo(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Surface* screen = SDL_SetVideoMode(
        FIXED_WIDTH, FIXED_HEIGHT, FIXED_DEPTH,
        SDL_HWSURFACE | SDL_DOUBLEBUF
    );
    if (!screen) {
        LOG_error("Failed to set video mode: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_ShowCursor(0);
    PLAT_clearVideo(screen);
    return screen;
}
```

**Notes:**
- Called once at application startup
- Thread: Main thread only
- Do not block for extended periods

---

### PLAT_quitVideo

```c
void PLAT_quitVideo(void);
```

Cleanup and shutdown video subsystem.

**Responsibilities:**
- Release video surfaces
- Call SDL_Quit() or equivalent
- Restore framebuffer state if modified

**Example:**
```c
void PLAT_quitVideo(void) {
    SDL_Quit();
}
```

**Notes:**
- Called once at application shutdown
- Thread: Main thread only

---

### PLAT_clearVideo

```c
void PLAT_clearVideo(SDL_Surface* screen);
```

Fill the given surface with black (0x0000).

**Parameters:**
- `screen`: Surface to clear

**Example:**
```c
void PLAT_clearVideo(SDL_Surface* screen) {
    SDL_FillRect(screen, NULL, 0);
}
```

**Notes:**
- Called between scene transitions
- Must not flip/present the screen
- Thread: Main thread only

---

### PLAT_clearAll

```c
void PLAT_clearAll(void);
```

Clear all video buffers (front and back buffers).

**Responsibilities:**
- Clear both framebuffers for double-buffered displays
- Prevents residual image artifacts on screen transitions

**Example:**
```c
void PLAT_clearAll(void) {
    PLAT_clearVideo(screen);
    PLAT_flip(screen, 0);
    PLAT_clearVideo(screen);
}
```

**Notes:**
- Called during major state changes (menu ↔ game)
- Thread: Main thread only

---

### PLAT_setVsync

```c
void PLAT_setVsync(int vsync);
```

Configure vsync behavior.

**Parameters:**
- `vsync`: One of VSYNC_OFF, VSYNC_LENIENT, VSYNC_STRICT

**Constants:**
- `VSYNC_OFF` (0): No frame pacing, run as fast as possible
- `VSYNC_LENIENT` (1): Target 60 FPS, allow occasional drops (default)
- `VSYNC_STRICT` (2): Hard vsync, wait for vblank every frame

**Example:**
```c
static int current_vsync = VSYNC_LENIENT;

void PLAT_setVsync(int vsync) {
    current_vsync = vsync;
    // Configure hardware vsync if supported
}
```

**Notes:**
- Called from settings menu and frontend initialization
- Thread: Main thread only

---

### PLAT_resizeVideo

```c
SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch);
```

Create a new surface for dynamic resolution changes.

**Parameters:**
- `w`: New width in pixels
- `h`: New height in pixels
- `pitch`: Bytes per scanline

**Returns:**
- New SDL_Surface matching the requested dimensions, or NULL on failure

**Example:**
```c
SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch) {
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, FIXED_DEPTH,
                                0xF800, 0x07E0, 0x001F, 0);
}
```

**Notes:**
- Called by Minarch when libretro core reports resolution change
- Must match pixel format of main screen
- Thread: Main thread only

---

### PLAT_setVideoScaleClip

```c
void PLAT_setVideoScaleClip(int x, int y, int width, int height);
```

Define the clip region for scaled video output.

**Parameters:**
- `x`, `y`: Top-left corner of clip region
- `width`, `height`: Dimensions of clip region

**Example:**
```c
static SDL_Rect scale_clip;

void PLAT_setVideoScaleClip(int x, int y, int width, int height) {
    scale_clip.x = x;
    scale_clip.y = y;
    scale_clip.w = width;
    scale_clip.h = height;
}
```

**Notes:**
- Used for aspect ratio correction and centering
- Thread: Main thread only

---

### PLAT_flip

```c
void PLAT_flip(SDL_Surface* screen, int sync);
```

Present the rendered frame to the display.

**Parameters:**
- `screen`: Surface to present
- `sync`: If non-zero, wait for vsync; if zero, return immediately

**Example:**
```c
void PLAT_flip(SDL_Surface* screen, int sync) {
    if (sync && current_vsync != VSYNC_OFF) {
        SDL_Flip(screen); // Blocks for vsync
    } else {
        SDL_Flip(screen);
    }
}
```

**Notes:**
- **Critical performance path**: Optimize for minimal latency
- Thread: Main thread only
- Called 60 times per second during gameplay

---

### PLAT_vsync

```c
void PLAT_vsync(int remaining);
```

Sleep until next frame time to maintain target framerate.

**Parameters:**
- `remaining`: Microseconds remaining until next frame

**Example:**
```c
void PLAT_vsync(int remaining) {
    if (remaining > 0) {
        usleep(remaining);
    }
}
```

**Notes:**
- Called when frame finishes early and vsync is enabled
- Thread: Main thread only

---

### PLAT_supportsOverscan

```c
int PLAT_supportsOverscan(void);
```

Query if platform supports overscan adjustment.

**Returns:**
- Non-zero if overscan is supported, zero otherwise

**Example:**
```c
int PLAT_supportsOverscan(void) {
    return 0; // Most platforms don't support this
}
```

---

## Audio Subsystem

### SND_init

```c
void SND_init(double sample_rate, double frame_rate);
```

Initialize audio subsystem. **Implemented in api.c, uses PLAT_pickSampleRate.**

**Parameters:**
- `sample_rate`: Requested sample rate (e.g., 44100.0)
- `frame_rate`: Game frame rate for buffering calculations (e.g., 60.0)

**Notes:**
- Platform provides PLAT_pickSampleRate() to select compatible rate
- api.c handles SDL audio initialization and callback setup

---

### SND_batchSamples

```c
size_t SND_batchSamples(const SND_Frame* frames, size_t frame_count);
```

Submit audio samples for playback. **Implemented in api.c.**

**Parameters:**
- `frames`: Array of stereo frames (int16_t left, int16_t right)
- `frame_count`: Number of frames to submit

**Returns:**
- Number of frames actually queued

**Notes:**
- Called by libretro core from audio callback
- Thread: Audio thread (from libretro core)

---

### PLAT_pickSampleRate

```c
int PLAT_pickSampleRate(int requested, int max);
```

Choose an audio sample rate supported by the hardware.

**Parameters:**
- `requested`: Preferred sample rate (typically from libretro core)
- `max`: Maximum acceptable sample rate

**Returns:**
- Supported sample rate closest to `requested`, not exceeding `max`

**Example:**
```c
int PLAT_pickSampleRate(int requested, int max) {
    const int supported[] = {22050, 32000, 44100, 48000};
    int best = 22050;

    for (int i = 0; i < 4; i++) {
        if (supported[i] <= max && supported[i] <= requested) {
            best = supported[i];
        }
    }
    return best;
}
```

**Notes:**
- Called during SND_init()
- Common rates: 22050, 32000, 44100, 48000
- Thread: Main thread only

---

## Input Subsystem

### PLAT_initInput

```c
void PLAT_initInput(void);
```

Initialize input devices.

**Responsibilities:**
- Initialize SDL joystick subsystem (SDL_Init(SDL_INIT_JOYSTICK))
- Open device-specific input interfaces (/dev/input, GPIO, etc.)
- Allocate input state tracking structures

**Example:**
```c
void PLAT_initInput(void) {
    SDL_Init(SDL_INIT_JOYSTICK);
    if (SDL_NumJoysticks() > 0) {
        SDL_JoystickOpen(0);
    }
    SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
}
```

**Notes:**
- Called once at application startup
- Thread: Main thread only

---

### PLAT_quitInput

```c
void PLAT_quitInput(void);
```

Cleanup input subsystem.

**Responsibilities:**
- Close joystick devices
- Release input resources
- Restore input device state if modified

**Example:**
```c
void PLAT_quitInput(void) {
    // SDL cleanup handled by SDL_Quit()
}
```

**Notes:**
- Called once at application shutdown
- Thread: Main thread only

---

### PLAT_pollInput

```c
void PLAT_pollInput(void);
```

Sample all input devices and update global `PAD_Context pad` structure.

**Responsibilities:**
- Poll SDL events (SDL_PollEvent)
- Read device-specific inputs
- Update `pad.is_pressed` with currently held buttons
- Update `pad.just_pressed` with newly pressed buttons (this frame only)
- Update `pad.just_released` with newly released buttons (this frame only)
- Update `pad.laxis` and `pad.raxis` for analog sticks

**Example:**
```c
void PLAT_pollInput(void) {
    static uint32_t prev = 0;
    uint32_t curr = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_SPACE:  curr |= (1 << BTN_ID_A); break;
                case SDLK_LSHIFT: curr |= (1 << BTN_ID_B); break;
                // ... map all buttons
            }
        }
    }

    pad.is_pressed = curr;
    pad.just_pressed = curr & ~prev;
    pad.just_released = prev & ~curr;
    prev = curr;

    // Analog sticks (example)
    pad.laxis.x = 0;
    pad.laxis.y = 0;
    pad.raxis.x = 0;
    pad.raxis.y = 0;
}
```

**Global State:**
```c
extern PAD_Context pad;

typedef struct PAD_Context {
    int is_pressed;           // Bitmask of BTN_ID_*
    int just_pressed;         // Bitmask of BTN_ID_*
    int just_released;        // Bitmask of BTN_ID_*
    int just_repeated;        // Bitmask (for UI navigation)
    uint32_t repeat_at[BTN_ID_COUNT];
    PAD_Axis laxis;           // Left analog stick
    PAD_Axis raxis;           // Right analog stick
} PAD_Context;
```

**Notes:**
- **Critical performance path**: Called every frame (60 Hz)
- Thread: Main thread only
- Must complete in <1ms

---

### PLAT_shouldWake

```c
int PLAT_shouldWake(void);
```

Check if a wake event occurred during sleep.

**Returns:**
- Non-zero if device should wake from sleep, zero otherwise

**Example:**
```c
int PLAT_shouldWake(void) {
    // Check power button, lid open, etc.
    return lid.is_open || power_button_pressed();
}
```

**Notes:**
- Called by power management during sleep state
- Should be lightweight (non-blocking)

---

## Power Management

### PLAT_initLid

```c
void PLAT_initLid(void);
```

Initialize lid/flip detection (for clamshell devices).

**Responsibilities:**
- Open GPIO or input device for lid switch
- Initialize `lid` global context

**Example:**
```c
LID_Context lid = {0};

void PLAT_initLid(void) {
    // Check if device has lid switch
    if (access("/dev/input/event-lid", F_OK) == 0) {
        lid.has_lid = 1;
        lid.is_open = 1;
    }
}
```

**Notes:**
- Called during GFX_init()
- Thread: Main thread only

---

### PLAT_lidChanged

```c
int PLAT_lidChanged(int* state);
```

Check if lid state changed since last call.

**Parameters:**
- `state`: Output parameter, set to 1 if open, 0 if closed

**Returns:**
- Non-zero if state changed, zero otherwise

**Example:**
```c
int PLAT_lidChanged(int* state) {
    static int prev_state = 1;
    int curr_state = read_lid_gpio();

    *state = curr_state;
    if (curr_state != prev_state) {
        prev_state = curr_state;
        return 1;
    }
    return 0;
}
```

**Notes:**
- Called periodically by power management
- Thread: Main thread or background thread

---

### PLAT_getBatteryStatus

```c
void PLAT_getBatteryStatus(int* is_charging, int* charge);
```

Query battery level and charging state.

**Parameters:**
- `is_charging`: Output, set to 1 if charging, 0 otherwise
- `charge`: Output, set to quantized charge level

**Charge Levels:**
- 0, 10, 20, 40, 60, 80, 100

**Example:**
```c
void PLAT_getBatteryStatus(int* is_charging, int* charge) {
    FILE* fp = fopen("/sys/class/power_supply/battery/capacity", "r");
    if (fp) {
        int raw_charge;
        fscanf(fp, "%d", &raw_charge);
        fclose(fp);

        // Quantize to standard levels
        if (raw_charge <= 5)       *charge = 0;
        else if (raw_charge <= 15) *charge = 10;
        else if (raw_charge <= 30) *charge = 20;
        else if (raw_charge <= 50) *charge = 40;
        else if (raw_charge <= 70) *charge = 60;
        else if (raw_charge <= 90) *charge = 80;
        else                        *charge = 100;
    } else {
        *charge = 100; // Assume full if can't read
    }

    FILE* fp2 = fopen("/sys/class/power_supply/battery/status", "r");
    if (fp2) {
        char status[16];
        fscanf(fp2, "%s", status);
        fclose(fp2);
        *is_charging = (strcmp(status, "Charging") == 0);
    } else {
        *is_charging = 0;
    }
}
```

**Notes:**
- Called by background thread every 30 seconds
- Must not block for extended periods
- Thread: Background thread (battery_pt)

---

### PLAT_enableBacklight

```c
void PLAT_enableBacklight(int enable);
```

Turn display backlight on or off.

**Parameters:**
- `enable`: 1 to enable, 0 to disable

**Example:**
```c
void PLAT_enableBacklight(int enable) {
    FILE* fp = fopen("/sys/class/backlight/backlight/bl_power", "w");
    if (fp) {
        fprintf(fp, "%d\n", enable ? 0 : 1); // 0=on, 1=off
        fclose(fp);
    }
}
```

**Notes:**
- Called during sleep/wake transitions
- Thread: Main thread only

---

### PLAT_powerOff

```c
void PLAT_powerOff(void);
```

Initiate system shutdown.

**Responsibilities:**
- Save any pending state
- Trigger OS shutdown (via system() call or direct syscall)
- Does not return

**Example:**
```c
void PLAT_powerOff(void) {
    sync(); // Flush filesystem buffers
    system("poweroff");
}
```

**Notes:**
- Called when user selects shutdown or battery is critical
- Thread: Main thread only

---

### PLAT_setCPUSpeed

```c
void PLAT_setCPUSpeed(int speed);
```

Adjust CPU frequency and governor.

**Parameters:**
- `speed`: One of CPU_SPEED_MENU, CPU_SPEED_POWERSAVE, CPU_SPEED_NORMAL, CPU_SPEED_PERFORMANCE

**Constants:**
```c
enum {
    CPU_SPEED_MENU = 0,        // Minimal (e.g., 600 MHz)
    CPU_SPEED_POWERSAVE = 1,   // Balanced (e.g., 800 MHz)
    CPU_SPEED_NORMAL = 2,      // Standard (e.g., 1000 MHz)
    CPU_SPEED_PERFORMANCE = 3, // Maximum (e.g., 1500 MHz)
};
```

**Example:**
```c
void PLAT_setCPUSpeed(int speed) {
    const char* freq_mhz[] = {"600000", "800000", "1000000", "1500000"};

    FILE* fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "w");
    if (fp) {
        fprintf(fp, "%s\n", freq_mhz[speed]);
        fclose(fp);
    }
}
```

**Notes:**
- Called during mode transitions (menu → game, fast-forward, etc.)
- Failure is non-fatal; log and continue
- Thread: Main thread only

---

### PLAT_setRumble

```c
void PLAT_setRumble(int strength);
```

Control vibration motor intensity.

**Parameters:**
- `strength`: Intensity from 0 (off) to 100 (maximum)

**Example:**
```c
void PLAT_setRumble(int strength) {
    // Write to PWM device or GPIO
    FILE* fp = fopen("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", "w");
    if (fp) {
        fprintf(fp, "%d\n", strength * 10); // Scale to hardware range
        fclose(fp);
    }
}
```

**Notes:**
- Optional feature; can be no-op if hardware lacks rumble
- Thread: Main thread only

---

## Platform Information

### PLAT_getModel

```c
char* PLAT_getModel(void);
```

Return human-readable model name.

**Returns:**
- Static string with device model (e.g., "Miyoo Mini", "RG35XX")

**Example:**
```c
char* PLAT_getModel(void) {
    return "MyDevice v1.0";
}
```

**Notes:**
- Used for logging and display in about screen
- Thread: Any

---

### PLAT_isOnline

```c
int PLAT_isOnline(void);
```

Check if network connectivity is available.

**Returns:**
- Non-zero if online, zero otherwise

**Example:**
```c
int PLAT_isOnline(void) {
    // Check WiFi interface status
    return access("/sys/class/net/wlan0/operstate", F_OK) == 0;
}
```

**Notes:**
- Optional feature for WiFi-enabled devices
- Thread: Any

---

### PLAT_setDateTime

```c
int PLAT_setDateTime(int y, int m, int d, int h, int i, int s);
```

Set system date and time.

**Parameters:**
- `y`: Year (e.g., 2025)
- `m`: Month (1-12)
- `d`: Day (1-31)
- `h`: Hour (0-23)
- `i`: Minute (0-59)
- `s`: Second (0-59)

**Returns:**
- 0 on success, non-zero on failure

**Example:**
```c
int PLAT_setDateTime(int y, int m, int d, int h, int i, int s) {
    struct tm tm = {0};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = i;
    tm.tm_sec = s;

    time_t t = mktime(&tm);
    struct timeval tv = {t, 0};
    return settimeofday(&tv, NULL);
}
```

**Notes:**
- Called from settings menu
- Thread: Main thread only

---

## Graphics Rendering

### PLAT_setNearestNeighbor

```c
void PLAT_setNearestNeighbor(int enabled);
```

Toggle between nearest-neighbor and bilinear filtering.

**Parameters:**
- `enabled`: 1 for nearest-neighbor (sharp), 0 for bilinear (smooth)

**Example:**
```c
static int use_nearest = 1;

void PLAT_setNearestNeighbor(int enabled) {
    use_nearest = enabled;
}
```

**Notes:**
- Affects PLAT_getScaler() selection
- Thread: Main thread only

---

### PLAT_setSharpness

```c
void PLAT_setSharpness(int sharpness);
```

Set sharpness filter strength.

**Parameters:**
- `sharpness`: One of SHARPNESS_SHARP, SHARPNESS_CRISP, SHARPNESS_SOFT

**Constants:**
```c
enum {
    SHARPNESS_SHARP = 0,
    SHARPNESS_CRISP = 1,
    SHARPNESS_SOFT = 2,
};
```

**Example:**
```c
static int current_sharpness = SHARPNESS_SOFT;

void PLAT_setSharpness(int sharpness) {
    current_sharpness = sharpness;
}
```

**Notes:**
- Affects PLAT_blitRenderer() behavior
- Thread: Main thread only

---

### PLAT_setEffectColor

```c
void PLAT_setEffectColor(int color);
```

Set color for scanline/grid effects (RGB565).

**Parameters:**
- `color`: RGB565 color value

**Example:**
```c
static uint16_t effect_color = 0x0000; // Black

void PLAT_setEffectColor(int color) {
    effect_color = color;
}
```

---

### PLAT_setEffect

```c
void PLAT_setEffect(int effect);
```

Enable visual effect overlay.

**Parameters:**
- `effect`: One of EFFECT_NONE, EFFECT_LINE, EFFECT_GRID

**Constants:**
```c
enum {
    EFFECT_NONE = 0,
    EFFECT_LINE = 1,  // Horizontal scanlines
    EFFECT_GRID = 2,  // Grid pattern
};
```

**Example:**
```c
static int current_effect = EFFECT_NONE;

void PLAT_setEffect(int effect) {
    current_effect = effect;
}
```

---

### PLAT_getScaler

```c
scaler_t PLAT_getScaler(GFX_Renderer* renderer);
```

Return function pointer for software scaling.

**Parameters:**
- `renderer`: Rendering context with source/destination dimensions

**Returns:**
- Function pointer of type `scaler_t`

**Scaler Function Signature:**
```c
typedef void (*scaler_t)(uint16_t* src, int src_pitch,
                         uint16_t* dst, int dst_pitch,
                         int width, int height);
```

**Common Scalers (from scaler.c):**
- `scale1x_c16`: 1x nearest-neighbor
- `scale2x_c16`: 2x nearest-neighbor
- `scale3x_c16`: 3x nearest-neighbor
- `scaleBilinear_c16`: Bilinear interpolation
- Platform-specific optimized scalers (NEON, etc.)

**Example:**
```c
scaler_t PLAT_getScaler(GFX_Renderer* renderer) {
    if (renderer->scale == 1) return scale1x_c16;
    if (renderer->scale == 2) return scale2x_c16;
    if (renderer->scale == 3) return scale3x_c16;
    return use_nearest ? scale2x_c16 : scaleBilinear_c16;
}
```

**Notes:**
- Called once per resolution change
- Thread: Main thread only

---

### PLAT_blitRenderer

```c
void PLAT_blitRenderer(GFX_Renderer* renderer);
```

Execute the full rendering pipeline: scale, filter, and blit to screen.

**Parameters:**
- `renderer`: Rendering context

**GFX_Renderer Structure:**
```c
typedef struct GFX_Renderer {
    void* src;           // Source buffer (core output)
    void* dst;           // Destination buffer (screen)
    void* blit;          // Intermediate buffer (optional)
    double aspect;       // Aspect ratio (0=integer, -1=fullscreen, else ratio)
    int scale;           // Integer scale factor

    int true_w, true_h;  // True output dimensions

    int src_x, src_y;    // Source region
    int src_w, src_h;
    int src_p;           // Source pitch

    int dst_x, dst_y;    // Destination region
    int dst_w, dst_h;
    int dst_p;           // Destination pitch
} GFX_Renderer;
```

**Example:**
```c
void PLAT_blitRenderer(GFX_Renderer* renderer) {
    scaler_t scaler = PLAT_getScaler(renderer);

    // Scale core output to intermediate buffer
    scaler((uint16_t*)renderer->src, renderer->src_p,
           (uint16_t*)renderer->blit, renderer->dst_p,
           renderer->src_w, renderer->src_h);

    // Apply sharpness filter
    if (current_sharpness == SHARPNESS_SHARP) {
        apply_sharp_filter(renderer->blit, renderer->dst_w, renderer->dst_h);
    }

    // Apply effect overlay
    if (current_effect == EFFECT_LINE) {
        apply_scanlines(renderer->blit, renderer->dst_w, renderer->dst_h);
    }

    // Blit to screen
    SDL_Rect src_rect = {0, 0, renderer->dst_w, renderer->dst_h};
    SDL_Rect dst_rect = {renderer->dst_x, renderer->dst_y,
                          renderer->dst_w, renderer->dst_h};
    SDL_BlitSurface(blit_surface, &src_rect, screen, &dst_rect);
}
```

**Notes:**
- **Critical performance path**: Called every frame during gameplay
- Optimize with SIMD (NEON, SSE) where possible
- Thread: Main thread only

---

### PLAT_initOverlay

```c
SDL_Surface* PLAT_initOverlay(void);
```

Initialize hardware overlay layer (optional).

**Returns:**
- SDL_Surface for overlay, or NULL if unsupported

**Example:**
```c
SDL_Surface* PLAT_initOverlay(void) {
    // Create transparent overlay surface for OSD
    return SDL_CreateRGBSurface(SDL_SRCALPHA, FIXED_WIDTH, FIXED_HEIGHT, 32,
                                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}
```

**Notes:**
- Used for battery/volume indicators without disturbing game screen
- Thread: Main thread only

---

### PLAT_quitOverlay

```c
void PLAT_quitOverlay(void);
```

Cleanup overlay resources.

**Example:**
```c
void PLAT_quitOverlay(void) {
    if (overlay_surface) {
        SDL_FreeSurface(overlay_surface);
        overlay_surface = NULL;
    }
}
```

---

### PLAT_enableOverlay

```c
void PLAT_enableOverlay(int enable);
```

Show or hide hardware overlay.

**Parameters:**
- `enable`: 1 to show, 0 to hide

**Example:**
```c
void PLAT_enableOverlay(int enable) {
    if (overlay_surface) {
        SDL_SetAlpha(overlay_surface, enable ? SDL_SRCALPHA : 0, 255);
    }
}
```

---

## Data Types

### PAD_Context

```c
typedef struct PAD_Context {
    int is_pressed;           // Bitmask of currently held buttons
    int just_pressed;         // Bitmask of buttons pressed this frame
    int just_released;        // Bitmask of buttons released this frame
    int just_repeated;        // Bitmask for UI navigation repeat
    uint32_t repeat_at[BTN_ID_COUNT];  // Timestamp for next repeat
    PAD_Axis laxis;           // Left analog stick
    PAD_Axis raxis;           // Right analog stick
} PAD_Context;

typedef struct PAD_Axis {
    int x;  // -32768 to 32767 (left to right)
    int y;  // -32768 to 32767 (up to down)
} PAD_Axis;
```

**Global Instance:**
```c
extern PAD_Context pad;
```

---

### LID_Context

```c
typedef struct LID_Context {
    int has_lid;   // 1 if device has lid switch, 0 otherwise
    int is_open;   // 1 if lid is open, 0 if closed
} LID_Context;
```

**Global Instance:**
```c
extern LID_Context lid;
```

---

### GFX_Renderer

See [PLAT_blitRenderer](#plat_blitrenderer) for structure definition.

---

### SND_Frame

```c
typedef struct SND_Frame {
    int16_t left;   // Left channel sample
    int16_t right;  // Right channel sample
} SND_Frame;
```

---

## Constants

### Button IDs

```c
enum {
    BTN_ID_A,
    BTN_ID_B,
    BTN_ID_X,
    BTN_ID_Y,
    BTN_ID_START,
    BTN_ID_SELECT,
    BTN_ID_MENU,
    BTN_ID_L1, BTN_ID_L2, BTN_ID_L3,
    BTN_ID_R1, BTN_ID_R2, BTN_ID_R3,
    BTN_ID_UP, BTN_ID_DOWN, BTN_ID_LEFT, BTN_ID_RIGHT,
    BTN_ID_PLUS,
    BTN_ID_MINUS,
    BTN_ID_COUNT
};
```

**Usage:**
```c
if (pad.is_pressed & (1 << BTN_ID_A)) {
    // A button is held
}

if (pad.just_pressed & (1 << BTN_ID_START)) {
    // Start button just pressed this frame
}
```

---

### Display Constants

Defined in `workspace/<platform>/platform/platform.h`:

```c
#define FIXED_WIDTH     640   // Native horizontal resolution
#define FIXED_HEIGHT    480   // Native vertical resolution
#define FIXED_BPP       2     // Bytes per pixel (2 or 4)
#define FIXED_SCALE     2     // UI scaling factor (1, 2, or 3)
#define FIXED_DEPTH     (FIXED_BPP * 8)
#define FIXED_PITCH     (FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE      (FIXED_PITCH * FIXED_HEIGHT)
```

---

### Color Masks

```c
#define RGBA_MASK_AUTO   0x0, 0x0, 0x0, 0x0
#define RGBA_MASK_565    0xF800, 0x07E0, 0x001F, 0x0000
#define RGBA_MASK_8888   0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
```

---

### Resource Paths

```c
#define SDCARD_PATH     "/mnt/sdcard"
#define RES_PATH        "/mnt/sdcard/.system/res"
```

---

## Helper Macros

### FALLBACK_IMPLEMENTATION

```c
#define FALLBACK_IMPLEMENTATION __attribute__((weak))
```

**Usage:**
```c
// Provide default implementation that platforms can override
FALLBACK_IMPLEMENTATION void PLAT_setRumble(int strength) {
    // No-op default
}
```

---

### Logging

```c
#define LOG_debug(fmt, ...) LOG_note(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_info(fmt, ...)  LOG_note(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_warn(fmt, ...)  LOG_note(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_error(fmt, ...) LOG_note(LOG_ERROR, fmt, ##__VA_ARGS__)

void LOG_note(int level, const char* fmt, ...);
```

**Usage:**
```c
LOG_info("Initialized video: %dx%d\n", FIXED_WIDTH, FIXED_HEIGHT);
LOG_error("Failed to open audio device: %s\n", SDL_GetError());
```

---

## Best Practices

### Performance

1. **Optimize critical paths**: PLAT_pollInput(), PLAT_flip(), PLAT_blitRenderer()
2. **Use SIMD**: Leverage NEON/SSE for scaling and blitting
3. **Minimize allocations**: Pre-allocate buffers during init
4. **Profile regularly**: Measure frame time and identify hotspots

### Error Handling

1. **Log failures**: Use LOG_error() for diagnostics
2. **Fail gracefully**: Return sensible defaults on errors
3. **Don't crash**: Validate inputs and handle edge cases
4. **Document limits**: Note hardware constraints in comments

### Thread Safety

1. **Single-threaded default**: Most functions called from main thread
2. **Document exceptions**: PLAT_getBatteryStatus() called from background thread
3. **Avoid blocking**: Complete quickly or defer to background tasks
4. **Use atomics carefully**: If sharing state across threads

### Portability

1. **Avoid hardcoding**: Use constants from platform.h
2. **Check availability**: Test for optional features at runtime
3. **Provide fallbacks**: Implement FALLBACK_IMPLEMENTATION where sensible
4. **Document assumptions**: Note Linux-specific or SDL-specific code

---

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System design overview
- [PORTING.md](PORTING.md) - Step-by-step porting guide
- `workspace/all/common/api.c` - Reference implementations
- `workspace/all/common/scaler.c` - Software scaling functions
- Existing platform implementations for examples

---

**This API enables MinUI to run across diverse handheld devices while maintaining
a consistent user experience. Careful implementation ensures smooth emulation,
responsive controls, and efficient power management.**
