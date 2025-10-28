# Phase 2 Configuration System Integration Status

This document tracks which configuration options are fully integrated, partially integrated, or not yet integrated.

## Integration Status Legend

- ✅ **INTEGRATED**: Configuration option is fully applied and functional
- ⚠️ **PARTIAL**: Configuration option is applied but may need additional work
- ❌ **NOT INTEGRATED**: Configuration option is parsed but not yet applied
- 🔧 **NEEDS PLATFORM WORK**: Requires platform-specific integration

---

## Configuration Options Status

### Display Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `display_scale` | ✅ INTEGRATED | minarch.c:4730-4738 | Maps to screen_scaling: aspect/fullscreen/integer/native |
| `display_sharpness` | ✅ INTEGRATED | minarch.c:4716-4722 | Maps to screen_sharpness: sharp/crisp/soft |
| `display_vsync` | ✅ INTEGRATED | minarch.c:4725-4727<br>minui.c:1333-1334 | Applied in both emulator and menu |
| `graphics_backend` | ❌ NOT INTEGRATED | - | Requires USE_GFX_BACKEND=1 flag and backend selection code |

### Audio Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `audio_latency` | 🔧 NEEDS PLATFORM WORK | - | SAMPLES is compile-time constant (#define)<br>Would require changing to runtime variable |
| `audio_sample_rate` | ❌ NOT INTEGRATED | - | SND_init uses fixed sample rate detection |

### Performance Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `thread_video` | ✅ INTEGRATED | minarch.c:4741-4742 | Enables threaded video rendering if USE_FRAME_QUEUE=1 |
| `cpu_speed_menu` | 🔧 NEEDS PLATFORM WORK | - | Platform uses enum values, not direct MHz |
| `cpu_speed_powersave` | 🔧 NEEDS PLATFORM WORK | - | Platform uses enum values, not direct MHz |
| `cpu_speed_normal` | 🔧 NEEDS PLATFORM WORK | - | Platform uses enum values, not direct MHz |
| `cpu_speed_performance` | 🔧 NEEDS PLATFORM WORK | - | Platform uses enum values, not direct MHz |

### Emulation Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `fast_forward_speed` | ✅ INTEGRATED | minarch.c:4744-4746 | Correctly maps config value (2-10x) to internal (0-9) |
| `savestate_slots` | ✅ INTEGRATED | minarch.c:3009,4755-4758 | MENU_SLOT_COUNT now configurable (1-10 slots) |
| `rewind_enabled` | ❌ NOT IMPLEMENTED | - | No rewind system exists (would need state buffer management) |
| `frame_skip` | ❌ NOT IMPLEMENTED | - | Feature not implemented in minarch (only commented code exists) |

### UI Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `show_fps` | ✅ INTEGRATED | minarch.c:4749-4751 | Maps to show_debug variable |
| `show_battery` | ✅ INTEGRATED | api.c:766-804 | Conditionally shows/hides battery indicator |
| `menu_timeout` | ✅ INTEGRATED | minarch.c:4283-4421 | Auto-closes menu after N seconds of inactivity (0=never) |

### Path Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `rom_path` | ❌ NOT INTEGRATED | - | File browser needs to use custom path |
| `bios_path` | ❌ NOT INTEGRATED | - | Core loading needs to check custom BIOS path |
| `saves_path` | ❌ NOT INTEGRATED | - | Save system needs to use custom path |

### Debug Settings

| Option | Status | Location | Notes |
|--------|--------|----------|-------|
| `debug` | ⚠️ PARTIAL | minarch.c:4760-4762<br>minui.c:1335-1337 | Used for logging, not globally available |
| `log_level` | ✅ INTEGRATED | api.c:39-46 | LOG_note checks log_level before output (0=error, 1=warn, 2=info, 3=debug) |

---

## Currently Integrated (Working)

These configuration options are fully applied and functional:

1. **display_scale** - Controls game scaling mode (aspect/fullscreen/integer/native)
2. **display_sharpness** - Controls rendering sharpness (sharp/crisp/soft)
3. **display_vsync** - Controls vertical sync (0=off, 1=lenient, 2=strict)
4. **show_fps** - Shows FPS counter overlay during gameplay
5. **fast_forward_speed** - Controls fast-forward multiplier (2x-10x)
6. **thread_video** - Enables threaded video rendering (when USE_FRAME_QUEUE=1)
7. **savestate_slots** - Number of save state slots (1-10, default 8)
8. **show_battery** - Show/hide battery indicator (0=hide, 1=show)
9. **log_level** - Control logging verbosity (0=error, 1=warn, 2=info, 3=debug)
10. **menu_timeout** - Auto-close menu after N seconds of inactivity (0=never timeout)

**New in this update**: menu_timeout
**Previous update**: savestate_slots, show_battery, log_level

---

## Needs Additional Work

### Audio Latency (audio_latency)

**Problem**: SDL audio buffer size (SAMPLES) is a compile-time constant:
```c
#ifndef SAMPLES
	#define SAMPLES 512 // default
#endif
```

**Solution Options**:
1. Change SAMPLES to a runtime variable initialized from config
2. Rebuild with different SAMPLES value for different profiles
3. Add dynamic audio buffer resizing in SND_init()

**Recommended**: Option 1 - Convert SAMPLES to runtime variable

### CPU Speed Settings

**Problem**: Platforms use enum values (CPU_SPEED_MENU, etc.) mapped to hardcoded frequencies:
```c
void PLAT_setCPUSpeed(int speed) {
	int freq = 0;
	switch (speed) {
		case CPU_SPEED_MENU: freq = 504000; break;
		case CPU_SPEED_NORMAL: freq = 1296000; break;
		// ...
	}
}
```

**Solution**: Platform-specific override table:
```c
// In platform.c PLAT_initVideo():
if (config && config->cpu_speed_menu > 0) {
	cpu_freq_override[CPU_SPEED_MENU] = config->cpu_speed_menu * 1000;
}
```

**Status**: Requires per-platform integration

### Save State Slots (savestate_slots)

**Current**: Hard-coded to 4 slots

**Needs**: State system to read config->savestate_slots and allocate accordingly

**Location**: State management code in minarch.c

### Rewind (rewind_enabled)

**Current**: Not initialized from config

**Needs**: Rewind system to check config and enable/disable accordingly

**Status**: Rewind system may not be implemented yet

### Frame Skip (frame_skip)

**Current**: Not applied from config

**Needs**: Frame timing code to read config->frame_skip setting

**Location**: Core run loop in minarch.c

### Path Settings (rom_path, bios_path, saves_path)

**Current**: Uses hardcoded or detected paths

**Needs**: File system code to check config for custom paths

**Locations**:
- ROM path: Menu file browser
- BIOS path: Core initialization
- Saves path: Save file management

---

## Code Locations

### Currently Applied Settings

**minarch.c lines 4711-4757**:
```c
#ifdef USE_CONFIG_SYSTEM
	minui_config_t* minui_config = CONFIG_get();
	if (minui_config) {
		// Apply display settings
		if (minui_config->display_sharpness == DISPLAY_SHARP) {
			screen_sharpness = SHARPNESS_SHARP;
		// ... (more settings)
	}
#endif
```

**minui.c lines 1330-1343**:
```c
#ifdef USE_CONFIG_SYSTEM
	minui_config_t* minui_config = CONFIG_get();
	if (minui_config && minui_config->display_vsync >= 0) {
		GFX_setVsync(minui_config->display_vsync);
		// ...
	}
#endif
```

---

## Testing Each Option

### Tested and Working

1. **display_sharpness**: Set `display_sharpness=sharp` in config and verify rendering is sharper
2. **display_scale**: Set `display_scale=fullscreen` and verify game stretches to fill screen
3. **display_vsync**: Set `display_vsync=0` and verify tearing occurs (vsync disabled)
4. **show_fps**: Set `show_fps=1` and verify FPS counter appears
5. **fast_forward_speed**: Set `fast_forward_speed=8` and test fast-forward is 8x
6. **thread_video**: Set `thread_video=1` (requires USE_FRAME_QUEUE=1 build)

### Not Yet Testable

Options marked as ❌ or 🔧 cannot be tested until integration code is added.

---

## Next Steps for Complete Integration

### Phase 1: Quick Wins (Already Done ✅)
- [x] display_scale
- [x] display_sharpness
- [x] display_vsync
- [x] show_fps
- [x] fast_forward_speed
- [x] thread_video

### Phase 2: Moderate Effort
- [ ] debug flag (make globally accessible)
- [x] log_level (integrate with LOG_* functions)
- [x] savestate_slots
- [ ] frame_skip (NOT IMPLEMENTED - feature doesn't exist)
- [x] show_battery
- [x] menu_timeout

### Phase 3: Complex (Platform-Specific)
- [ ] audio_latency (requires refactoring)
- [ ] CPU speed overrides (per-platform)
- [ ] Path settings (file system integration)

### Phase 4: Feature Flags
- [ ] graphics_backend (requires USE_GFX_BACKEND=1)
- [ ] rewind_enabled (NOT IMPLEMENTED - no rewind system exists)

---

## Summary

**Total Config Options**: 23
- ✅ **Integrated and Working**: 10 (43%)
- ⚠️ **Partial/Debug Only**: 1 (4%)
- 🔧 **Needs Platform Work**: 5 (22%)
- ❌ **Not Yet Integrated**: 5 (22%)
- ❌ **Not Implemented**: 2 (9%)

**Current Focus**: 10 core display, performance, and UI options are working. The remaining options require more extensive integration work across multiple subsystems.

**Recommendation**: Continue integrating remaining options based on priority: path settings (rom_path, bios_path, saves_path), button_swap, and analog_sensitivity are next logical candidates.

---

Last Updated: 2025-10-28
Branch: claude/rg35xx-phase2-integration-011CUXxjQqMKtRrphKw937Rj
