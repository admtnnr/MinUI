# MinUI Configuration Guide

MinUI follows a **zero-configuration philosophy** - it works perfectly out of the box
with sensible defaults. However, for advanced users and specific hardware requirements,
MinUI supports optional file-based configuration.

## Table of Contents

1. [Philosophy](#philosophy)
2. [Quick Start](#quick-start)
3. [Configuration File Format](#configuration-file-format)
4. [Graphics Settings](#graphics-settings)
5. [Audio Settings](#audio-settings)
6. [Emulation Settings](#emulation-settings)
7. [Performance Tuning](#performance-tuning)
8. [Path Configuration](#path-configuration)
9. [UI Customization](#ui-customization)
10. [Debugging Options](#debugging-options)
11. [Platform-Specific Notes](#platform-specific-notes)
12. [Troubleshooting](#troubleshooting)

---

## Philosophy

MinUI's configuration system is designed around these principles:

- **Zero-config by default**: Without a config file, MinUI works perfectly
- **Sensible defaults**: All settings have reasonable defaults matching current behavior
- **No UI complexity**: Configuration is file-based, not menu-based
- **Forward compatible**: Unknown settings are ignored, allowing config portability
- **Optional only**: Configuration enables advanced customization, not basic functionality

If you're happy with how MinUI works, you don't need a config file at all!

---

## Quick Start

### Creating a Configuration File

1. Copy the example configuration:
   ```bash
   cp /mnt/sdcard/.userdata/minui.conf.example /mnt/sdcard/.userdata/minui.conf
   ```

2. Edit the file with your preferred text editor:
   ```bash
   nano /mnt/sdcard/.userdata/minui.conf
   ```

3. Uncomment and modify only the settings you want to change

4. Save and restart MinUI

### Example: Enable FPS Counter

```ini
# Show FPS counter
show_fps=1
```

### Example: Use Framebuffer Backend for Better Performance

```ini
# Use direct framebuffer for maximum speed
graphics_backend=fbdev
display_scale=aspect
```

### Example: Reduce Audio Latency

```ini
# Lower audio latency for rhythm games
audio_latency=32
```

---

## Configuration File Format

### Location

- **Default path**: `/mnt/sdcard/.userdata/minui.conf`
- **Platform-specific**: `/mnt/sdcard/.userdata/<platform>/minui.conf` (if it exists)

### Syntax

```ini
# Lines starting with # are comments
# Format: key=value (no spaces around =)
# Empty lines are ignored

# This is a comment
graphics_backend=sdl2
display_scale=aspect
```

### Rules

1. **One setting per line**: `key=value`
2. **No spaces around equals**: Use `key=value`, not `key = value`
3. **Comments start with #**: Everything after # is ignored
4. **Case-sensitive keys**: Use exact key names from documentation
5. **Unknown keys ignored**: Forward compatible with future versions

---

## Graphics Settings

Graphics settings control how MinUI renders video output.

### graphics_backend

**Controls**: Which graphics backend to use for rendering
**Values**: `auto`, `sdl2`, `sdl2_hw`, `fbdev`, `drm`
**Default**: `auto`

```ini
graphics_backend=auto
```

**Options explained:**

- **`auto`**: Let MinUI choose automatically (recommended)
  - Tries backends in order: fbdev → sdl2_hw → sdl2
  - Best for most users

- **`sdl2`**: Use SDL2 software rendering
  - Compatible with all devices
  - Slower than other backends
  - Recommended only as fallback

- **`sdl2_hw`**: Use SDL2 hardware acceleration
  - GPU-accelerated where available
  - Better performance than software SDL2
  - May not work on all devices

- **`fbdev`**: Use Linux framebuffer directly
  - Maximum performance
  - Bypasses SDL overhead
  - Linux-only, requires /dev/fb0

- **`drm`**: Use DRM/KMS (not yet implemented)
  - Modern graphics stack
  - Future backend for mainline kernels

**When to change:**
- If you experience poor performance, try `fbdev`
- If `fbdev` doesn't work, use `sdl2_hw`
- Use `sdl2` only if nothing else works

### display_scale

**Controls**: How game video is scaled to screen
**Values**: `aspect`, `fullscreen`, `integer`, `native`
**Default**: `aspect`

```ini
display_scale=aspect
```

**Options explained:**

- **`aspect`**: Maintain aspect ratio with black bars
  - No distortion
  - May have letterboxing/pillarboxing
  - Recommended for most games

- **`fullscreen`**: Stretch to fill entire screen
  - No black bars
  - May distort image (circles become ovals)
  - Use if you don't mind distortion

- **`integer`**: Use integer scaling factors only
  - Sharp, pixel-perfect scaling
  - May result in small image
  - Best for pixel art games

- **`native`**: No scaling, 1:1 pixels
  - Smallest image size
  - Perfect pixel accuracy
  - Use for GBA/GB on small screens

**When to change:**
- For pixel-perfect scaling: `integer`
- To eliminate black bars: `fullscreen`
- For maximum screen usage without distortion: `aspect`

### display_sharpness

**Controls**: Sharpness/smoothing filter applied to video
**Values**: `sharp`, `crisp`, `soft`
**Default**: `soft`

```ini
display_sharpness=soft
```

**Options explained:**

- **`sharp`**: No filtering, nearest-neighbor scaling
  - Maximum sharpness
  - May look pixelated/blocky
  - Best for pixel art purists

- **`crisp`**: Light filtering
  - Balanced appearance
  - Slight smoothing

- **`soft`**: Bilinear filtering
  - Smooth appearance
  - Reduces pixelation
  - Better for 3D games

**When to change:**
- For pixel art games (NES, SNES): `sharp`
- For 3D games (N64, PSX): `soft`
- For compromise: `crisp`

### display_vsync

**Controls**: Vertical sync mode
**Values**: `0` (off), `1` (lenient), `2` (strict)
**Default**: `1`

```ini
display_vsync=1
```

**Options explained:**

- **`0` (off)**: No frame pacing, run as fast as possible
  - Lowest latency
  - May cause screen tearing
  - Higher power consumption

- **`1` (lenient)**: Target 60 FPS, allow occasional drops
  - Good balance
  - Prevents most tearing
  - Recommended for most users

- **`2` (strict)**: Hard vsync, wait for vblank
  - No tearing
  - May cause stuttering if performance is marginal
  - Use if you hate tearing

**When to change:**
- If you see screen tearing: increase to `2`
- For minimum input lag: set to `0`
- For smooth gameplay: keep at `1`

---

## Audio Settings

Audio settings control latency and quality.

### audio_latency

**Controls**: Audio buffer size in milliseconds
**Range**: `32` to `256`
**Default**: `64`

```ini
audio_latency=64
```

**How it works:**
- Lower = less lag, but may cause audio stuttering/crackling
- Higher = more lag, but more stable audio
- Sweet spot depends on device performance

**Recommended values:**
- **Fast devices**: `32` (minimum lag)
- **Average devices**: `64` (balanced)
- **Slow devices**: `128` (stable audio)
- **Problematic devices**: `256` (maximum stability)

**When to change:**
- If audio crackles/stutters: increase by 32 until stable
- For rhythm games (DDR, Guitar Hero): decrease as low as stable
- If audio feels delayed: decrease carefully

### audio_sample_rate

**Controls**: Audio sampling rate in Hz
**Values**: `0` (auto), `22050`, `32000`, `44100`, `48000`
**Default**: `0`

```ini
audio_sample_rate=0
```

**When to change:**
- Usually leave at `0` (auto)
- Change only if you experience audio pitch problems
- Some devices may require specific rates

---

## Emulation Settings

Settings that affect emulation behavior.

### savestate_slots

**Controls**: Number of save state slots per game
**Range**: `1` to `10`
**Default**: `4`

```ini
savestate_slots=4
```

**What it means:**
- Each game can have this many quick-save slots
- More slots = more storage used
- 4 slots is usually plenty

**Storage impact:**
- Each save state is typically 1-5 MB
- 4 slots × 100 games = up to 2 GB

### frame_skip

**Controls**: Frame skipping mode
**Values**: `0` (off), `1` (auto), `2-4` (skip 1-3 frames)
**Default**: `0`

```ini
frame_skip=0
```

**Options:**
- **`0`**: No frame skipping, best quality
- **`1`**: Auto frame skip when needed
- **`2`**: Skip 1 frame (render every other frame)
- **`3`**: Skip 2 frames (render every 3rd frame)
- **`4`**: Skip 3 frames (render every 4th frame)

**When to use:**
- Only if games run too slowly
- Try `1` (auto) first
- Manual skip (`2-4`) for specific problem games

### rewind_enabled

**Controls**: Enable rewind feature
**Values**: `0` (off), `1` (on)
**Default**: `0`

```ini
rewind_enabled=0
```

**Warning:**
- Uses significant RAM and CPU
- May reduce performance
- Increases power consumption
- Only enable if you need it

### fast_forward_speed

**Controls**: Fast-forward multiplier
**Range**: `0` (unlimited), `2` to `10`
**Default**: `3`

```ini
fast_forward_speed=3
```

**What it means:**
- `0`: Run as fast as possible
- `3`: 3x normal speed (180% game speed)
- `10`: 10x normal speed

---

## Performance Tuning

Advanced performance settings.

### thread_video

**Controls**: Enable threaded video rendering
**Values**: `0` (off), `1` (on)
**Default**: `0`

```ini
thread_video=0
```

**What it does:**
- Separates video rendering into dedicated thread
- Can improve performance on multi-core devices
- Not all platforms support this

**When to enable:**
- If games drop frames despite good CPU
- If your device has multiple cores
- Test carefully - may not always help

**When NOT to enable:**
- Single-core devices
- If unsure
- If you experience crashes

### CPU Speed Overrides

**Controls**: CPU frequency for different modes
**Range**: `0` (use default) or frequency in MHz
**Default**: `0`

```ini
cpu_speed_menu=0
cpu_speed_powersave=0
cpu_speed_normal=0
cpu_speed_performance=0
```

**⚠️ WARNING ⚠️**
- Only change if you know your device's supported frequencies!
- Incorrect values may cause crashes or damage
- Leave at `0` to use platform defaults

**Example (RG35XX):**
```ini
cpu_speed_menu=600         # UI navigation
cpu_speed_powersave=800    # Light emulation
cpu_speed_normal=1200      # Standard emulation
cpu_speed_performance=1500 # Demanding cores
```

**When to change:**
- If you need more performance
- To reduce power consumption
- To match specific core requirements

---

## Path Configuration

Customize where MinUI looks for files.

### rom_path

**Default**: `/mnt/sdcard/Roms`

```ini
rom_path=/mnt/sdcard/Roms
```

### bios_path

**Default**: `/mnt/sdcard/Bios`

```ini
bios_path=/mnt/sdcard/Bios
```

### saves_path

**Default**: `/mnt/sdcard/Saves`

```ini
saves_path=/mnt/sdcard/Saves
```

**Notes:**
- Paths must exist before launching MinUI
- Use absolute paths
- No trailing slashes

---

## UI Customization

Visual and interface settings.

### show_fps

**Controls**: Display FPS counter
**Values**: `0` (off), `1` (on)
**Default**: `0`

```ini
show_fps=1
```

**What it shows:**
- Frames per second in top-left corner
- Useful for performance testing

### show_battery

**Controls**: Display battery indicator
**Values**: `0` (off), `1` (on)
**Default**: `1`

```ini
show_battery=1
```

### menu_timeout

**Controls**: Auto-hide in-game menu (seconds)
**Range**: `0` (never), `1` to `300`
**Default**: `0`

```ini
menu_timeout=10
```

**What it does:**
- Automatically hides in-game menu after inactivity
- `0` = never hide (default)
- Useful if menu is in the way

---

## Debugging Options

For troubleshooting and development.

### debug

**Controls**: Enable debug mode
**Values**: `0` (off), `1` (on)
**Default**: `0`

```ini
debug=1
```

**What it enables:**
- Additional logging
- Debug features
- Performance metrics

### log_level

**Controls**: Verbosity of logging
**Values**: `0` (errors), `1` (warnings), `2` (info), `3` (debug)
**Default**: `1`

```ini
log_level=2
```

**Log levels:**
- **`0`**: Only critical errors
- **`1`**: Errors and warnings (recommended)
- **`2`**: Informational messages
- **`3`**: Everything (very verbose)

---

## Platform-Specific Notes

### Miyoo Mini / Mini+

```ini
# Recommended settings
graphics_backend=fbdev
display_scale=aspect
audio_latency=64
```

### RG35XX / RG35XX+

```ini
# Recommended settings
graphics_backend=fbdev
display_scale=integer
cpu_speed_normal=1200
```

### RGB30

```ini
# Recommended settings (has custom scaler)
graphics_backend=sdl2
display_scale=aspect
```

### TrimUI Smart

```ini
# Recommended settings
graphics_backend=fbdev
audio_latency=96
```

---

## Troubleshooting

### Problem: Games run slowly

**Try these settings:**
```ini
frame_skip=1
cpu_speed_performance=1500  # Adjust for your device
thread_video=1  # If multi-core
```

### Problem: Screen tearing

**Try these settings:**
```ini
display_vsync=2
graphics_backend=fbdev
```

### Problem: Audio crackling

**Try these settings:**
```ini
audio_latency=128  # Increase until stable
audio_sample_rate=44100
```

### Problem: Poor video quality

**Try these settings:**
```ini
display_sharpness=sharp
display_scale=integer
```

### Problem: Config not loading

1. Check file location: `/mnt/sdcard/.userdata/minui.conf`
2. Check file permissions: `chmod 644 minui.conf`
3. Check syntax: no spaces around `=`
4. Enable debug logging:
   ```ini
   debug=1
   log_level=3
   ```

### Problem: Crashes after changing config

1. Remove config file and restart
2. Add settings back one at a time
3. Check for invalid values (especially CPU frequencies)
4. Verify paths exist

---

## Best Practices

### Start Small
- Begin with default config (no file)
- Add settings one at a time
- Test each change

### Document Changes
- Add comments to your config explaining why you changed settings
- Keep backup of working config

### Performance Testing
- Enable `show_fps=1` when tuning
- Test with demanding games
- Adjust gradually

### Safety
- Never guess CPU frequencies
- Backup before major changes
- Keep example config for reference

---

## Advanced Examples

### Maximum Performance Profile

```ini
# Squeeze every frame
graphics_backend=fbdev
display_scale=integer
display_vsync=0
frame_skip=1
thread_video=1
cpu_speed_performance=1800  # Device-specific!
```

### Maximum Quality Profile

```ini
# Best visual quality
graphics_backend=sdl2_hw
display_scale=aspect
display_sharpness=sharp
display_vsync=2
frame_skip=0
```

### Battery Saver Profile

```ini
# Extend battery life
cpu_speed_normal=800
cpu_speed_performance=1000
display_vsync=1
thread_video=0
audio_latency=128
```

### Rhythm Game Profile

```ini
# Minimize input and audio lag
graphics_backend=fbdev
display_vsync=0
audio_latency=32
cpu_speed_performance=1500
```

---

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture
- [PORTING.md](PORTING.md) - Platform porting guide
- [API.md](API.md) - Platform API reference

---

**Remember**: MinUI is designed to work perfectly without configuration. Only
customize if you have specific needs or requirements!
