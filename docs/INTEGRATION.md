# Phase 2 Integration Guide

This guide shows platform maintainers how to integrate the new Phase 2 systems
(graphics backends, frame queue, and configuration) into their platforms.

## Table of Contents

1. [Overview](#overview)
2. [Graphics Backend Integration](#graphics-backend-integration)
3. [Frame Queue Integration](#frame-queue-integration)
4. [Configuration System Integration](#configuration-system-integration)
5. [Complete Integration Example](#complete-integration-example)
6. [Testing](#testing)
7. [Migration Checklist](#migration-checklist)

---

## Overview

Phase 2 introduces three new systems that platforms can adopt:

1. **Graphics Backend Abstraction** - Pluggable rendering backends
2. **Frame Queue** - Thread-safe video pipeline
3. **Configuration System** - Optional file-based settings

All three systems are **opt-in** and **backward compatible**. Platforms can
adopt them incrementally without breaking existing functionality.

---

## Graphics Backend Integration

### Step 1: Choose Your Backend(s)

Decide which graphics backend(s) your platform should use:

- **SDL2 Software (`gfx_backend_sdl2`)**: Compatible, slower
- **SDL2 Hardware (`gfx_backend_sdl2_hw`)**: GPU-accelerated (future)
- **Framebuffer (`gfx_backend_fbdev_impl`)**: Direct /dev/fb0, fast
- **DRM/KMS (`gfx_backend_drm`)**: Modern stack (future)

**Recommendation:**
- Try `fbdev` first if on Linux
- Fall back to `sdl2` if `fbdev` fails
- Use configuration to allow user override

### Step 2: Register Backends

In your `workspace/<platform>/platform/platform.c`, update `PLAT_initVideo()`:

```c
#include "gfx_backend.h"

// Add near top of file
extern const gfx_backend_t gfx_backend_fbdev_impl;

SDL_Surface* PLAT_initVideo(void) {
    // Register available backends in order of preference
    gfx_backend_register(&gfx_backend_fbdev_impl);  // Try fbdev first
    gfx_backend_register(&gfx_backend_sdl2);        // SDL2 as fallback

    // Initialize with preferred backend
    gfx_backend_context_t* gfx_ctx = gfx_backend_init(
        NULL,  // NULL = use first registered backend
        FIXED_WIDTH,
        FIXED_HEIGHT,
        GFX_FORMAT_RGB565  // or GFX_FORMAT_XRGB8888
    );

    if (!gfx_ctx) {
        LOG_error("Failed to initialize any graphics backend\n");
        return NULL;
    }

    // Get SDL surface for compatibility
    // (Some backends like SDL2 provide this directly)
    const gfx_backend_t* backend = gfx_backend_get_active();
    LOG_info("Using graphics backend: %s\n", backend->name);

    // For SDL2 backend, get the screen surface
    if (strcmp(backend->name, "sdl2") == 0) {
        return SDL_GetVideoSurface();
    }

    // For non-SDL backends, create a wrapper surface
    // This maintains compatibility with existing code
    int pitch;
    void* pixels = backend->get_framebuffer(gfx_ctx, &pitch);

    if (pixels) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            pixels,
            FIXED_WIDTH,
            FIXED_HEIGHT,
            FIXED_DEPTH,
            pitch,
            0xF800, 0x07E0, 0x001F, 0  // RGB565 masks
        );
        return surface;
    }

    // Fallback: create a normal surface
    return SDL_CreateRGBSurface(SDL_SWSURFACE, FIXED_WIDTH, FIXED_HEIGHT,
                                FIXED_DEPTH, 0xF800, 0x07E0, 0x001F, 0);
}
```

### Step 3: Update PLAT_flip()

Use the backend for presentation:

```c
void PLAT_flip(SDL_Surface* screen, int sync) {
    const gfx_backend_t* backend = gfx_backend_get_active();
    gfx_backend_context_t* ctx = gfx_backend_get_context();

    if (backend && ctx) {
        backend->set_vsync(ctx, sync);
        backend->present(ctx, screen->pixels,
                         screen->w, screen->h, screen->pitch);
    } else {
        // Fallback to SDL
        SDL_Flip(screen);
    }
}
```

### Step 4: Update PLAT_quitVideo()

Clean up backend resources:

```c
void PLAT_quitVideo(void) {
    gfx_backend_shutdown();
    SDL_Quit();
}
```

### Step 5: Configuration Support (Optional)

Allow users to override backend selection:

```c
#include "config.h"

SDL_Surface* PLAT_initVideo(void) {
    // Load configuration
    minui_config_t* config = config_load(NULL);

    // Register backends
    gfx_backend_register(&gfx_backend_fbdev_impl);
    gfx_backend_register(&gfx_backend_sdl2);

    // Use configured backend or auto
    const char* backend_name = NULL;
    if (config && strcmp(config->graphics_backend, "auto") != 0) {
        backend_name = config->graphics_backend;
        LOG_info("Using configured backend: %s\n", backend_name);
    }

    gfx_backend_context_t* gfx_ctx = gfx_backend_init(
        backend_name,  // NULL or specific backend
        FIXED_WIDTH,
        FIXED_HEIGHT,
        GFX_FORMAT_RGB565
    );

    config_free(config);

    // ... rest of initialization
}
```

### Step 6: Update Makefile (for fbdev)

Add the fbdev backend to your build if using it:

In `workspace/<platform>/platform/makefile.env`:

```makefile
# Add fbdev backend for Linux platforms
ifeq ($(PLATFORM),miyoomini)
    USE_FBDEV_BACKEND := 1
endif

ifdef USE_FBDEV_BACKEND
    CFLAGS += -DUSE_FBDEV_BACKEND
    LDFLAGS += -lrt
endif
```

In `workspace/all/common/makefile` (or appropriate location):

```makefile
COMMON_SRC = api.c gfx_backend.c config.c

ifdef USE_FBDEV_BACKEND
    COMMON_SRC += gfx_backend_fbdev.c
endif
```

---

## Frame Queue Integration

Frame queue enables threaded video rendering. This is optional and should only
be enabled if your platform benefits from it.

### When to Use Frame Queue

- Multi-core devices
- When SDL2 threading causes issues
- When separating rendering improves performance

### When NOT to Use

- Single-core devices
- If threaded rendering causes crashes
- If single-threaded is fast enough

### Integration in minarch.c

**Step 1: Add Global State**

```c
#include "frame_queue.h"

// Near top of file
static frame_queue_t* video_queue = NULL;
static pthread_t render_thread;
static int use_threaded_video = 0;
```

**Step 2: Render Thread Function**

```c
static void* render_thread_func(void* arg) {
    LOG_info("Render thread started\n");

    while (!quit && !frame_queue_is_shutdown(video_queue)) {
        // Wait for next frame (16ms timeout for 60 FPS)
        frame_handle_t handle = frame_queue_acquire_read(video_queue, 16);

        if (handle != FRAME_INVALID) {
            // Get frame data
            const uint8_t* buffer = frame_queue_get_buffer(video_queue, handle);
            frame_info_t info;
            frame_queue_get_info(video_queue, handle, &info);

            // Present using backend or SDL
            const gfx_backend_t* backend = gfx_backend_get_active();
            gfx_backend_context_t* ctx = gfx_backend_get_context();

            if (backend && ctx) {
                backend->present(ctx, (void*)buffer,
                                 info.width, info.height, info.pitch);
            } else {
                // Fallback to SDL
                memcpy(screen->pixels, buffer, info.height * info.pitch);
                SDL_Flip(screen);
            }

            // Release frame
            frame_queue_release(video_queue, handle);
        }
    }

    LOG_info("Render thread stopped\n");
    return NULL;
}
```

**Step 3: Initialize Frame Queue**

```c
// In video initialization, after SDL setup:
static void init_video_threaded(int width, int height) {
    // Check if threaded video is enabled
    minui_config_t* config = config_load(NULL);
    use_threaded_video = config ? config->thread_video : 0;
    config_free(config);

    if (!use_threaded_video) {
        LOG_info("Threaded video disabled\n");
        return;
    }

    LOG_info("Initializing threaded video\n");

    // Create frame queue (triple buffering)
    video_queue = frame_queue_create(
        width,
        height,
        FRAME_FORMAT_RGB565,  // or FRAME_FORMAT_XRGB8888
        3  // Triple buffering
    );

    if (!video_queue) {
        LOG_error("Failed to create frame queue, disabling threaded video\n");
        use_threaded_video = 0;
        return;
    }

    // Start render thread
    if (pthread_create(&render_thread, NULL, render_thread_func, NULL) != 0) {
        LOG_error("Failed to create render thread\n");
        frame_queue_destroy(video_queue);
        video_queue = NULL;
        use_threaded_video = 0;
        return;
    }

    LOG_info("Threaded video initialized successfully\n");
}
```

**Step 4: Update Video Callback**

```c
// Libretro video refresh callback
static void retro_video_refresh(const void* data, unsigned width,
                                 unsigned height, size_t pitch) {
    if (!data) return;

    if (use_threaded_video && video_queue) {
        // Threaded path
        frame_handle_t handle = frame_queue_acquire_write(video_queue);

        if (handle != FRAME_INVALID) {
            uint8_t* dst = frame_queue_get_buffer(video_queue, handle);
            size_t copy_size = height * pitch;
            memcpy(dst, data, copy_size);
            frame_queue_submit(video_queue, handle);
        } else {
            // Queue full, drop frame (rare)
            LOG_warn("Dropped frame: queue full\n");
        }
    } else {
        // Single-threaded path (existing code)
        memcpy(screen->pixels, data, height * pitch);
        PLAT_flip(screen, 1);
    }
}
```

**Step 5: Cleanup**

```c
// In shutdown code:
static void cleanup_video_threaded(void) {
    if (use_threaded_video && video_queue) {
        LOG_info("Shutting down threaded video\n");

        frame_queue_shutdown(video_queue);
        pthread_join(render_thread, NULL);
        frame_queue_destroy(video_queue);

        video_queue = NULL;
        use_threaded_video = 0;
    }
}
```

---

## Configuration System Integration

The configuration system is completely optional and requires minimal changes.

### Step 1: Load Configuration in main()

In `workspace/all/minui/minui.c` or `workspace/all/minarch/minarch.c`:

```c
#include "config.h"

int main(int argc, char** argv) {
    // Load configuration early
    minui_config_t* config = config_load(NULL);  // NULL = default path

    if (!config) {
        LOG_warn("Failed to load config, using defaults\n");
        // Continue with hardcoded defaults
    } else {
        // Merge command-line arguments
        config_merge_args(config, argc, argv);

        // Print config if debug enabled
        if (config->debug) {
            config_print(config, LOG_INFO);
        }
    }

    // Pass config to subsystems (or use globals)
    // ...

    // Rest of initialization
    // ...

    // Cleanup
    if (config) {
        config_free(config);
    }

    return 0;
}
```

### Step 2: Use Configuration Values

Throughout your code, check config values instead of hardcoded defaults:

```c
// Example: Audio initialization
void init_audio(minui_config_t* config) {
    int sample_rate = config && config->audio_sample_rate > 0
        ? config->audio_sample_rate
        : 44100;  // Default

    int latency = config
        ? config->audio_latency
        : 64;  // Default

    SND_init(sample_rate, 60.0);
    // Apply latency setting...
}

// Example: FPS display
void render_ui(minui_config_t* config) {
    if (config && config->show_fps) {
        draw_fps_counter();
    }

    if (config && config->show_battery) {
        draw_battery_indicator();
    }
}
```

### Step 3: Optional Command-Line Support

```c
// Allow config overrides from command line
int main(int argc, char** argv) {
    minui_config_t* config = config_load(NULL);

    // Usage: ./minui --config-backend=fbdev --config-show-fps=1
    config_merge_args(config, argc, argv);

    // ...
}
```

---

## Complete Integration Example

Here's a full example showing all three systems together:

```c
// workspace/<platform>/platform/platform.c

#include "api.h"
#include "gfx_backend.h"
#include "frame_queue.h"
#include "config.h"

extern const gfx_backend_t gfx_backend_fbdev_impl;

static minui_config_t* global_config = NULL;

SDL_Surface* PLAT_initVideo(void) {
    // Load configuration
    global_config = config_load(NULL);

    // Register available backends
    gfx_backend_register(&gfx_backend_fbdev_impl);
    gfx_backend_register(&gfx_backend_sdl2);

    // Select backend from config or auto
    const char* backend_name = NULL;
    if (global_config && strcmp(global_config->graphics_backend, "auto") != 0) {
        backend_name = global_config->graphics_backend;
    }

    // Initialize graphics backend
    gfx_backend_context_t* ctx = gfx_backend_init(
        backend_name,
        FIXED_WIDTH,
        FIXED_HEIGHT,
        GFX_FORMAT_RGB565
    );

    if (!ctx) {
        LOG_error("Failed to initialize graphics backend\n");
        return NULL;
    }

    // Create SDL surface wrapper
    const gfx_backend_t* backend = gfx_backend_get_active();
    LOG_info("Active graphics backend: %s\n", backend->name);

    if (strcmp(backend->name, "sdl2") == 0) {
        return SDL_GetVideoSurface();
    }

    // For fbdev, create surface wrapper
    int pitch;
    void* pixels = backend->get_framebuffer(ctx, &pitch);
    if (pixels) {
        return SDL_CreateRGBSurfaceFrom(
            pixels, FIXED_WIDTH, FIXED_HEIGHT, FIXED_DEPTH, pitch,
            0xF800, 0x07E0, 0x001F, 0
        );
    }

    return SDL_CreateRGBSurface(SDL_SWSURFACE, FIXED_WIDTH, FIXED_HEIGHT,
                                FIXED_DEPTH, 0xF800, 0x07E0, 0x001F, 0);
}

void PLAT_flip(SDL_Surface* screen, int sync) {
    const gfx_backend_t* backend = gfx_backend_get_active();
    gfx_backend_context_t* ctx = gfx_backend_get_context();

    if (backend && ctx) {
        // Apply vsync from config
        int vsync = (global_config && global_config->display_vsync >= 0)
            ? global_config->display_vsync
            : sync;

        backend->set_vsync(ctx, vsync > 0);
        backend->present(ctx, screen->pixels, screen->w, screen->h, screen->pitch);
    } else {
        SDL_Flip(screen);
    }
}

void PLAT_quitVideo(void) {
    gfx_backend_shutdown();

    if (global_config) {
        config_free(global_config);
        global_config = NULL;
    }

    SDL_Quit();
}
```

---

## Testing

### Test Plan

1. **Backward Compatibility**
   - Test without config file (should work as before)
   - Test with empty config file
   - Test with invalid config values

2. **Graphics Backends**
   - Test each backend individually
   - Test backend fallback (fbdev → sdl2)
   - Test with invalid backend name

3. **Frame Queue**
   - Test single-threaded mode
   - Test threaded mode (if enabled)
   - Test frame drops under load

4. **Configuration**
   - Test all config options
   - Test invalid values (should clamp)
   - Test command-line overrides

### Testing Checklist

- [ ] Boot without config file
- [ ] Boot with valid config file
- [ ] Test graphics_backend=sdl2
- [ ] Test graphics_backend=fbdev
- [ ] Test graphics_backend=invalid (should fallback)
- [ ] Test thread_video=0
- [ ] Test thread_video=1
- [ ] Test display scale modes
- [ ] Test vsync settings
- [ ] Test audio latency settings
- [ ] Test FPS counter
- [ ] Test save states
- [ ] Test fast forward
- [ ] Check for memory leaks
- [ ] Check for crashes
- [ ] Verify performance

---

## Migration Checklist

Use this checklist to track your integration progress:

### Phase 1: Graphics Backend

- [ ] Choose backends for your platform
- [ ] Add backend registration to PLAT_initVideo()
- [ ] Update PLAT_flip() to use backend
- [ ] Update PLAT_quitVideo() for cleanup
- [ ] Update makefile if using fbdev
- [ ] Test all backends
- [ ] Document recommended backend

### Phase 2: Configuration System

- [ ] Add config_load() to initialization
- [ ] Replace hardcoded values with config lookups
- [ ] Test with and without config file
- [ ] Create platform-specific example config
- [ ] Document configuration options

### Phase 3: Frame Queue (Optional)

- [ ] Evaluate if threaded video is beneficial
- [ ] Add frame queue initialization
- [ ] Create render thread
- [ ] Update video callback
- [ ] Add cleanup code
- [ ] Test thoroughly
- [ ] Document performance impact

### Phase 4: Testing & Documentation

- [ ] Complete all testing checklist items
- [ ] Update platform README
- [ ] Add example configurations
- [ ] Document known issues
- [ ] Submit pull request

---

## Troubleshooting

### Graphics Backend Issues

**Problem:** Backend initialization fails

**Solution:**
- Check if /dev/fb0 exists (for fbdev)
- Verify SDL is initialized
- Check log output for errors
- Try different backend

**Problem:** Screen corruption

**Solution:**
- Verify pixel format matches hardware
- Check pitch calculation
- Ensure proper buffer alignment

### Frame Queue Issues

**Problem:** Crashes in threaded mode

**Solution:**
- Disable threaded video (thread_video=0)
- Check for thread-safety issues
- Verify proper cleanup

**Problem:** Frame drops

**Solution:**
- Increase buffer count (3 → 4)
- Check render thread priority
- Profile rendering performance

### Configuration Issues

**Problem:** Config not loading

**Solution:**
- Verify file exists at /mnt/sdcard/.userdata/minui.conf
- Check file permissions
- Enable debug logging
- Check for syntax errors

---

## Getting Help

If you encounter issues during integration:

1. Check existing platform implementations for examples
2. Review documentation in docs/
3. Enable debug logging (debug=1, log_level=3)
4. Ask in GitHub discussions
5. Submit detailed bug reports with logs

---

## Next Steps

After integration:

1. Test thoroughly on real hardware
2. Document platform-specific notes
3. Share performance results
4. Help improve documentation
5. Contribute optimizations

Good luck with your integration!
