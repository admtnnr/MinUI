# MinUI Architecture and Build Overview

This document provides a comprehensive overview of MinUI's architecture, covering
both the runtime system design and the build/packaging infrastructure. It is
intended as background reading for contributors extending the project with new
backends, platform targets, or core functionality.

## Table of Contents

1. [Runtime Architecture](#runtime-architecture)
   - [System Overview](#system-overview)
   - [Core Components](#core-components)
   - [Graphics Pipeline](#graphics-pipeline)
   - [Audio Subsystem](#audio-subsystem)
   - [Input System](#input-system)
   - [Power Management](#power-management)
2. [Repository Layout](#repository-layout)
3. [Workspace Structure](#workspace-structure)
4. [Build Pipeline](#build-pipeline)
5. [Development Workflow](#development-workflow)

---

## Runtime Architecture

### System Overview

MinUI employs a layered architecture optimized for embedded systems with limited
resources. The design prioritizes simplicity, efficiency, and portability across
diverse handheld gaming hardware.

```
┌─────────────────────────────────────────────────────────────┐
│                         User Interface                       │
│                    (MinUI Launcher - minui.c)                │
├─────────────────────────────────────────────────────────────┤
│                   Libretro Frontend                          │
│              (Minarch - minarch.c + libretro.h)              │
├─────────────────────────────────────────────────────────────┤
│              Platform Abstraction Layer (PAL)                │
│              (api.h/api.c + platform.h/platform.c)           │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐   │
│  │ Graphics │  Audio   │  Input   │  Power   │   I/O    │   │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘   │
├─────────────────────────────────────────────────────────────┤
│             Platform-Specific Implementations                │
│         (workspace/<device>/platform/platform.c)             │
├─────────────────────────────────────────────────────────────┤
│                  Hardware / OS Layer                         │
│            (SDL2, framebuffer, DRM, etc.)                    │
└─────────────────────────────────────────────────────────────┘
```

### Core Components

#### 1. MinUI Launcher (`workspace/all/minui/minui.c`)

The main user-facing application responsible for:
- **ROM browsing**: Scans SD card directories and displays game collections
- **Launch coordination**: Starts appropriate libretro cores via Minarch
- **Save state management**: Handles auto-save/resume functionality
- **Settings UI**: Provides minimal configuration without overwhelming users
- **Resource management**: Maintains low memory footprint

**Key Design Principles:**
- Zero-configuration philosophy: sensible defaults, no complex menus
- Fast startup: optimized initialization and caching
- Directory-based organization: automatic system detection from folder names

#### 2. Minarch Frontend (`workspace/all/minarch/minarch.c`)

The libretro frontend that bridges MinUI and emulator cores:
- **Core loading**: Dynamic loading of libretro cores via dlopen()
- **Emulation loop**: Main run loop coordinating input, video, and audio
- **In-game menu**: Quick access to save states, settings, and exit
- **State persistence**: Automatic save state creation and restoration
- **Threading support**: Optional threaded video rendering for performance

**Threading Model:**
```c
// Single-threaded (default):
//   Input → Core → Video → Audio → Present

// Multi-threaded (optional):
//   Main Thread: Input → Core → Queue Frame
//   Render Thread: Dequeue Frame → Scale → Present
```

**Current Limitation:** SDL2 requires creation and rendering in the same thread,
which conflicts with some threading optimizations. See Priority 1 recommendations
for proposed solutions.

#### 3. Platform Abstraction Layer (`workspace/all/common/api.h`)

Provides a unified interface isolating platform differences:

**Graphics API (`GFX_*`)**:
- `GFX_init()`: Initialize video subsystem
- `GFX_resize()`: Handle dynamic resolution changes
- `GFX_flip()`: Present rendered frame
- `GFX_clear()`: Clear framebuffer
- `GFX_setVsync()`: Configure vsync mode

**Audio API (`SND_*`)**:
- `SND_init()`: Initialize audio with sample rate
- `SND_batchSamples()`: Submit audio frames
- `SND_quit()`: Cleanup audio resources

**Input API (`PAD_*`)**:
- `PAD_init()`: Initialize input devices
- `PAD_poll()`: Sample current input state
- `PAD_justPressed()`: Detect button press events
- `PAD_isPressed()`: Check button hold state

**Power API (`PWR_*`)**:
- `PWR_getBattery()`: Query battery level
- `PWR_isCharging()`: Check charging status
- `PWR_setCPUSpeed()`: Set performance mode
- `PWR_enableAutosleep()`: Manage auto-sleep

**Platform API (`PLAT_*`)**:
- Device-specific implementations of above functions
- Handles hardware initialization and teardown
- Provides scaling, color format conversion, and special features

### Graphics Pipeline

The graphics subsystem is performance-critical and varies significantly by device.

**Current Architecture:**
```
Core Output (RGB565/XRGB8888)
    ↓
Format Conversion (if needed)
    ↓
Scaling (integer, aspect, or fullscreen)
    ↓
Sharpness Filter (sharp/crisp/soft)
    ↓
Effects (scanlines, grid)
    ↓
Present (SDL_Flip or direct framebuffer)
```

**Rendering Paths:**
1. **SDL2 Software**: Portable but slower, uses SDL surfaces
2. **SDL2 Hardware**: GPU-accelerated where available
3. **Direct Framebuffer**: Linux fbdev for maximum performance
4. **DRM/KMS**: Modern Linux graphics stack (future)

**Known Issues:**
- SDL2 performance varies significantly across devices
- Some platforms require direct framebuffer access for acceptable FPS
- Custom scalers needed for specific hardware (e.g., RGB30)
- Thread safety issues with multi-threaded rendering

**Recommended Improvements:** See Priority 1 - Graphics Abstraction Layer

### Audio Subsystem

Audio is handled through SDL's audio callback mechanism:

```c
// Simplified audio flow:
void audio_callback(void* userdata, uint8_t* stream, int len) {
    // Core produces samples
    core.audio_batch(samples, frame_count);

    // Mix into SDL buffer
    memcpy(stream, samples, len);
}
```

**Current Implementation:**
- Fixed buffer size with SDL_OpenAudio()
- Basic resampling for sample rate matching
- Single audio stream (stereo)

**Known Issues:**
- Callback bugs causing audio glitches on some platforms
- No support for dynamic latency adjustment
- Limited resampling quality
- Potential buffer underruns on slow devices

**Recommended Improvements:** See Priority 2 - Audio Subsystem Enhancement

### Input System

Input abstraction handles diverse control schemes:

**Input Sources:**
- Physical buttons (D-pad, face buttons, triggers)
- Analog sticks (left/right)
- Special buttons (menu, volume, power)

**Button Mapping:**
```c
typedef enum {
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
    BTN_ID_COUNT
} ButtonID;
```

**Input Processing:**
1. Platform polls hardware state (`PLAT_pollInput()`)
2. Raw state mapped to logical buttons
3. Repeat/hold detection applied
4. Event flags set (just_pressed, is_pressed, just_released)

**Current Limitations:**
- Some devices have hardwired joysticks that can't be remapped
- Limited support for hotplugging controllers
- No on-screen configuration UI

**Recommended Improvements:** See Priority 2 - Input Abstraction Enhancement

### Power Management

Power management is crucial for battery-powered handhelds:

**Features:**
- Battery level monitoring
- Charging state detection
- CPU frequency scaling (menu/powersave/normal/performance)
- Auto-sleep after inactivity
- Low battery warnings
- Graceful shutdown

**CPU Speed Modes:**
- `CPU_SPEED_MENU`: Minimal speed for UI navigation
- `CPU_SPEED_POWERSAVE`: Balanced efficiency
- `CPU_SPEED_NORMAL`: Standard emulation
- `CPU_SPEED_PERFORMANCE`: Maximum speed for demanding cores

**Implementation:**
```c
void PWR_update(int* dirty, int* show_setting,
                PWR_callback_t before_sleep,
                PWR_callback_t after_sleep);
```

Handles brightness/volume adjustments via hardware buttons, triggers sleep/wake
events, and updates battery indicator.

---

## Repository Layout

The root of the repository is split between assets that describe the runtime
filesystem shipped to devices and the cross-compiled software that runs on
those filesystems:

| Path | Description |
| ---- | ----------- |
| `skeleton/` | Golden copy of the SD card layout that is copied into `build/` at the start of a release. Contains the `BASE/`, `SYSTEM/`, and `EXTRAS/` trees that ship to users. |
| `workspace/` | Source code and build logic for MinUI executables, libretro cores, and platform-specific tooling. This directory is mounted into the cross-compilation container during builds. |
| `makefile` | Host-side entry point for assembling a release bundle. Handles staging the `skeleton/`, invoking workspace builds for each platform, and packaging `.zip` payloads. |
| `makefile.toolchain` | Utility makefile that fetches the per-platform Docker toolchains and launches an interactive or batch build inside them. |
| `releases/` | Populated during packaging. Each `make` run produces `MinUI-<date>-base.zip` and `MinUI-<date>-extras.zip` archives here. |

The `build/` directory is ephemeral and recreated for every release. It
starts as a copy of `skeleton/` and is gradually populated with freshly built
binaries, cores, and metadata before packaging.

## Workspace Structure

The `workspace/` directory is the root of all cross-compiled components. It
contains both platform-agnostic projects under `workspace/all/` and folders
per hardware target (for example `workspace/rg35xx/`).

### Shared Projects (`workspace/all/`)

* `minui/` – Main launcher responsible for browsing the SD card and starting
  libretro cores.
* `minarch/` – Libretro frontend that provides consistent in-game menus and
  emulator controls.
* `minput/`, `clock/`, `say/`, `syncsettings/` – Small companion utilities
  that ship in the `EXTRAS` tree.
* `common/` – Shared code (`api.c`, `scaler.c`, etc.) consumed by MinUI and
  Minarch.
* `cores/` – Platform-independent wrapper makefile used by per-platform
  builds to fetch and compile libretro cores.

Each project exposes a makefile that expects a `PLATFORM` name and inherits
compiler settings from `workspace/<platform>/platform/makefile.env`. The
makefiles compile into `workspace/all/<project>/build/<platform>/` and rely on
libraries like `libmsettings` that are built by the platform-specific portion
of the workspace.

### Platform-Specific Folders

Every platform folder (e.g. `workspace/rg35xx/`) includes:

* `makefile` – Top-level orchestration for platform assets. It builds helper
  apps (such as DinguxCommander), calls into `boot/` scripts, and formats
  README files via `workspace/all/readmes`.
* `platform/` – Defines compiler flags (`makefile.env`) and provides helper
  sources (e.g. `platform.c`) that are linked into MinUI and Minarch.
* `libmsettings/`, `keymon/`, `cores/`, etc. – Platform-specific toolchains
  and libretro core builds.
* Optional extras such as `overclock/`, `install/`, or `ramdisk/` that are
  staged into the SD card image during packaging.

The workspace root also includes `makefile`, which is executed inside the
cross-compilation container. It builds shared libraries, invokes each shared
project, compiles per-platform cores, and finally runs the platform `make` to
assemble staging artifacts.

## Build Pipeline

1. **Toolchain initialization** – `make PLATFORM=<name> build` on the host
   clones the platform-specific Docker toolchain (`union-<platform>-toolchain`)
   if necessary and runs it via `docker run`. The repository `workspace/`
   directory is bind-mounted as `/root/workspace` inside the container.
2. **Workspace build** – Inside the container, `make` executes
   `workspace/makefile`. The shared projects and platform-specific components
   compile using the cross-compiler provided by the toolchain. Outputs land in
   the mounted `workspace/` tree.
3. **Staging** – Back on the host, the top-level `make` target copies the
   `skeleton/` into `build/`, places built binaries under `build/SYSTEM/<platform>/`
   and `build/EXTRAS/...`, and performs special-case transformations (for
   example moving `.tmp_update` payloads for the Miyoo family).
4. **Packaging** – The `package` target zips the populated `BASE/`, `SYSTEM/`,
   and `EXTRAS/` trees into release archives and writes metadata such as
   `version.txt` and `commits.txt`.

The `setup`, `system`, `cores`, `special`, and `tidy` targets defined in the
root `makefile` orchestrate the staging details for each supported platform.
They ensure that fresh binaries from the workspace build are copied to the
correct SD card paths before zipping.

## Development Workflow

* To enter an interactive toolchain shell for a platform:

  ```sh
  make PLATFORM=<platform> shell
  ```

* To perform a full rebuild for a single platform and stage it into the
  release structure:

  ```sh
  make PLATFORM=<platform> build
  make PLATFORM=<platform> system cores
  ```

  Subsequent invocations of `make` at the repository root can target multiple
  platforms by setting the `PLATFORMS` variable or using the default list in
  the makefile.

Understanding these layers—`skeleton/` for filesystem templates,
`workspace/` for cross-compiled outputs, and the root makefiles for packaging—
provides the foundation needed to add new backend targets such as a QEMU-based
emulator configuration in future work.
