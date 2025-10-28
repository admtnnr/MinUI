# RG35XX Phase 2 Integration Guide

This document describes how Phase 2 systems (graphics backends, frame queue, and configuration) are integrated into the RG35XX platform with feature flags for controlled testing.

## Overview

The RG35XX integration uses **conditional compilation** to enable Phase 2 features:
- **Configuration System**: File-based settings (minui.conf)
- **Graphics Backend**: Optional backend abstraction (keeps hardware path by default)
- **Frame Queue**: Optional threaded video rendering

All features are **disabled by default** and can be enabled via build flags for testing.

---

## Quick Start

### Building with Configuration System Only (Recommended)

```bash
cd workspace
make PLATFORM=rg35xx USE_CONFIG_SYSTEM=1
```

This enables the configuration system while keeping all existing video/audio code unchanged.

### Building with All Phase 2 Features

```bash
make PLATFORM=rg35xx \
    USE_CONFIG_SYSTEM=1 \
    USE_FRAME_QUEUE=1 \
    DEBUG_PHASE2=1
```

### Building Stock (No Phase 2)

```bash
make PLATFORM=rg35xx
```

Standard build, no changes to existing behavior.

---

## Feature Flags

Feature flags are defined in `workspace/rg35xx/platform/phase2_flags.mk`:

### USE_CONFIG_SYSTEM

**Default**: `1` (enabled)
**Purpose**: Enables configuration file support

```makefile
USE_CONFIG_SYSTEM=1  # Enable config system
USE_CONFIG_SYSTEM=0  # Disable (stock behavior)
```

**When enabled:**
- Loads `/mnt/sdcard/.userdata/minui.conf` at startup
- Applies configuration settings (CPU speed, audio latency, etc.)
- Falls back to defaults if file missing
- No performance impact if config file doesn't exist

**Testing:**
1. Build with `USE_CONFIG_SYSTEM=1`
2. Copy `minui.conf.example` to `/mnt/sdcard/.userdata/minui.conf`
3. Edit settings
4. Launch MinUI and verify settings applied

### USE_GFX_BACKEND

**Default**: `0` (disabled)
**Purpose**: Enables graphics backend abstraction

```makefile
USE_GFX_BACKEND=1  # Enable backend system
USE_GFX_BACKEND=0  # Use existing hardware path (default)
```

**When enabled:**
- Allows selecting graphics backend via config: `graphics_backend=auto/sdl2/fbdev`
- Default `auto` keeps existing RG35XX hardware acceleration
- Useful for testing fallback paths

**Note:** RG35XX has custom hardware acceleration (ION + Display Engine). The existing path is already optimized, so this flag is mainly for development/testing.

### USE_FBDEV_BACKEND

**Default**: `0` (disabled)
**Purpose**: Compiles framebuffer backend

```makefile
USE_FBDEV_BACKEND=1  # Include fbdev backend
USE_FBDEV_BACKEND=0  # Exclude
```

**Requires**: `USE_GFX_BACKEND=1`

**When enabled:**
- Compiles `gfx_backend_fbdev.c`
- Allows `graphics_backend=fbdev` in config
- **Not recommended for RG35XX** - hardware path is faster

### USE_FRAME_QUEUE

**Default**: `0` (disabled)
**Purpose**: Enables threaded video rendering

```makefile
USE_FRAME_QUEUE=1  # Enable frame queue
USE_FRAME_QUEUE=0  # Single-threaded (default)
```

**When enabled:**
- Adds frame queue implementation
- Allows `thread_video=1` in config
- Requires pthread (`-lpthread` added automatically)
- May improve performance on multi-core devices

**RG35XX note:** Has dual-core Cortex-A9, may benefit from threading. Test carefully.

### DEBUG_PHASE2

**Default**: `0` (disabled)
**Purpose**: Verbose logging for Phase 2 systems

```makefile
DEBUG_PHASE2=1  # Enable debug logging
DEBUG_PHASE2=0  # Normal logging
```

**When enabled:**
- Adds `-DDEBUG_PHASE2 -DDEBUG` to CFLAGS
- Prints configuration loading details
- Logs backend selection
- Logs frame queue statistics

**Use for troubleshooting only** - generates verbose output.

---

## Build System Changes

### Modified Files

1. **`workspace/rg35xx/platform/phase2_flags.mk`** (new)
   - Defines all feature flags
   - Default values for each flag

2. **`workspace/rg35xx/platform/makefile.env`** (modified)
   - Includes `phase2_flags.mk`
   - Adds conditional CFLAGS based on flags
   - Adds `-lpthread` if frame queue enabled

3. **`workspace/rg35xx/platform/platform.c`** (modified)
   - Conditional includes for Phase 2 headers
   - Config loading in `PLAT_initVideo()`
   - Config cleanup in `PLAT_quitVideo()`
   - Preserves existing hardware acceleration path

### Build Process

```
1. User sets flags: USE_CONFIG_SYSTEM=1, etc.
2. phase2_flags.mk exports flag variables
3. makefile.env adds corresponding -D defines to CFLAGS
4. platform.c uses #ifdef to conditionally compile code
5. Common Phase 2 objects linked if enabled
```

---

## Testing Procedures

### Test 1: Stock Build (Baseline)

```bash
# Build without Phase 2
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx

# Deploy to device
# Test normal MinUI functionality
# Note FPS, responsiveness, battery life
```

**Expected:** Identical to current MinUI behavior.

### Test 2: Configuration System Only

```bash
# Build with config system
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx USE_CONFIG_SYSTEM=1

# Create config file on device:
cp minui.conf.example /mnt/sdcard/.userdata/minui.conf

# Edit config:
nano /mnt/sdcard/.userdata/minui.conf
# Set: debug=1, log_level=2, show_fps=1

# Launch MinUI
# Verify FPS counter appears
# Check logs for "Configuration loaded"
```

**Expected:**
- FPS counter visible if `show_fps=1`
- Config settings applied (CPU speed, audio latency, etc.)
- No performance regression

### Test 3: Configuration Profiles

```bash
# Test different optimization profiles from minui.conf.example

# Profile 1: Maximum Battery Life
cpu_speed_normal=900
display_vsync=2
audio_latency=96

# Profile 2: Balanced (recommended)
cpu_speed_normal=1000
display_vsync=1
audio_latency=64

# Profile 3: Maximum Performance
cpu_speed_normal=1200
display_vsync=0
audio_latency=32
```

**Measure:**
- FPS stability
- Audio quality (crackling?)
- Battery life
- Input lag

### Test 4: Frame Queue (Advanced)

```bash
# Build with frame queue support
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx \
    USE_CONFIG_SYSTEM=1 \
    USE_FRAME_QUEUE=1 \
    DEBUG_PHASE2=1

# Enable in config:
thread_video=1

# Launch demanding core (e.g., SNES or Genesis)
# Compare FPS to single-threaded
```

**Expected:**
- Possible FPS improvement (10-20%)
- Check logs for frame queue statistics
- Monitor CPU usage

**Warning:** If crashes occur, disable `thread_video`.

### Test 5: Debug Mode

```bash
# Build with debug
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx \
    USE_CONFIG_SYSTEM=1 \
    DEBUG_PHASE2=1

# Enable in config:
debug=1
log_level=3

# Launch and check logs
```

**Expected:** Verbose logging of all Phase 2 operations.

---

## Configuration File Usage

### Installation

```bash
# Copy example to active location
cp workspace/rg35xx/minui.conf.example /mnt/sdcard/.userdata/minui.conf

# Edit with preferred settings
nano /mnt/sdcard/.userdata/minui.conf
```

### Recommended Settings for RG35XX

```ini
# Good balance of performance and battery
graphics_backend=auto
display_scale=aspect
display_sharpness=sharp
display_vsync=1
audio_latency=64
cpu_speed_menu=600
cpu_speed_normal=1000
cpu_speed_performance=1200
show_battery=1
```

### Testing Different Backends

```ini
# Test software SDL2 (slower but compatible)
graphics_backend=sdl2

# Test framebuffer (basic, if fbdev compiled in)
graphics_backend=fbdev

# Use hardware acceleration (default, fastest)
graphics_backend=auto
```

---

## Performance Expectations

### Configuration System

**Overhead:** <2ms at startup
**Runtime impact:** None (settings loaded once)
**Memory:** ~4KB for config structure

### Frame Queue (if enabled)

**Latency:** May reduce by 10-20% on multi-core
**CPU:** Increases by ~5% (second thread)
**Memory:** ~6MB for triple buffering (3× 640×480×2)

**RG35XX has 256MB RAM**, so frame queue overhead is negligible.

---

## Troubleshooting

### Problem: Config not loading

**Symptoms:** Settings ignored, defaults used

**Solution:**
```bash
# Check file exists
ls -l /mnt/sdcard/.userdata/minui.conf

# Check permissions
chmod 644 /mnt/sdcard/.userdata/minui.conf

# Enable debug logging
debug=1
log_level=3

# Check for syntax errors in config file
```

### Problem: Build errors with Phase 2 enabled

**Symptoms:** Undefined references to config_*, gfx_backend_*, etc.

**Solution:**
```bash
# Ensure common workspace builds Phase 2 objects
# Check that workspace/all/common/ includes:
# - config.c
# - gfx_backend.c
# - frame_queue.c (if USE_FRAME_QUEUE=1)

# Clean rebuild
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx USE_CONFIG_SYSTEM=1
```

### Problem: Crashes with thread_video=1

**Symptoms:** MinUI crashes during gameplay

**Solution:**
```bash
# Disable threaded video
thread_video=0

# Or build without frame queue
make PLATFORM=rg35xx USE_CONFIG_SYSTEM=1 USE_FRAME_QUEUE=0
```

### Problem: Performance regression

**Symptoms:** Lower FPS than stock build

**Solution:**
```bash
# Disable Phase 2 features and compare
make PLATFORM=rg35xx clean
make PLATFORM=rg35xx  # Stock build

# If stock is faster, report issue with specifics:
# - Which feature flag caused regression
# - Which core/game affected
# - FPS comparison (stock vs Phase 2)
```

---

## Development Notes

### Code Structure

```c
// Conditional compilation pattern used throughout platform.c

#ifdef USE_CONFIG_SYSTEM
#include "config.h"
static minui_config_t* platform_config = NULL;
#endif

// In PLAT_initVideo():
#ifdef USE_CONFIG_SYSTEM
    platform_config = config_load(NULL);
    // Use config settings...
#endif

// In PLAT_quitVideo():
#ifdef USE_CONFIG_SYSTEM
    if (platform_config) {
        config_free(platform_config);
    }
#endif
```

### Adding New Configuration Options

1. Add setting to `workspace/all/common/config.h`
2. Add parsing in `workspace/all/common/config.c`
3. Use in `platform.c` via `#ifdef USE_CONFIG_SYSTEM`
4. Document in `minui.conf.example`

### Testing Checklist

- [ ] Stock build works (no Phase 2)
- [ ] Config system loads settings correctly
- [ ] Config defaults work without file
- [ ] FPS matches stock build
- [ ] Audio quality matches stock build
- [ ] Battery life comparable to stock
- [ ] All cores tested (GB, GBC, GBA, NES, SNES, Genesis, PSX)
- [ ] Save states work
- [ ] Fast-forward works
- [ ] Menu navigation smooth

---

## Performance Comparison

Test results should be documented here after integration testing:

| Configuration | FPS (SNES) | FPS (PSX) | Battery Life | Notes |
|---------------|------------|-----------|--------------|-------|
| Stock         | TBD        | TBD       | TBD          | Baseline |
| Config only   | TBD        | TBD       | TBD          | Should match stock |
| + Frame queue | TBD        | TBD       | TBD          | Test threading |
| + Debug       | TBD        | TBD       | TBD          | Logging overhead |

---

## Future Enhancements

Potential additions for RG35XX:

1. **Hardware-Accelerated Backend**
   - Wrap existing ION+DE path as custom backend
   - Allows runtime switching for testing

2. **Audio Enhancement**
   - Implement Phase 3 audio ring buffer
   - Test latency improvements

3. **Input Remapping**
   - Phase 3 input abstraction
   - Custom button mappings

---

## References

- [Phase 2 Overview](../../docs/REFACTORING_ROADMAP.md#phase-2-priority-1-refactoring)
- [Configuration Guide](../../docs/CONFIGURATION.md)
- [Integration Guide](../../docs/INTEGRATION.md)
- [API Reference](../../docs/API.md)

---

## Support

For issues with RG35XX Phase 2 integration:

1. Check this document's troubleshooting section
2. Enable debug logging (`debug=1`, `log_level=3`)
3. Compare with stock build
4. Report with:
   - Feature flags used
   - Config file (if any)
   - FPS comparison
   - Logs from debug mode

---

**Last Updated:** 2025-10-27
**Status:** Initial integration complete, ready for testing
**Branch:** `claude/rg35xx-phase2-integration-011CUXxjQqMKtRrphKw937Rj`
