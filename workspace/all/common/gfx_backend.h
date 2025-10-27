#ifndef GFX_BACKEND_H
#define GFX_BACKEND_H

/**
 * Graphics Backend Abstraction Layer
 *
 * This interface abstracts the graphics rendering backend to allow runtime or
 * compile-time selection of the optimal rendering path per device. This addresses
 * performance issues where SDL2 rendering is insufficient and direct framebuffer
 * access or DRM/KMS is preferred.
 *
 * Multiple forks have identified that SDL2 performance varies significantly across
 * devices, with some requiring custom scalers or direct framebuffer manipulation
 * for acceptable performance. This abstraction enables:
 *
 * - Runtime backend selection based on device capabilities
 * - Platform-specific optimizations without forking core code
 * - Future support for modern graphics stacks (DRM/KMS, Wayland, Vulkan)
 * - Simplified porting process for new devices
 *
 * Each backend implements this interface and registers itself during platform
 * initialization. The graphics subsystem selects the appropriate backend based
 * on platform configuration or runtime detection.
 */

#include <stdint.h>
#include <stddef.h>
#include "SDL/SDL.h"

// Forward declarations
typedef struct GFX_Renderer GFX_Renderer;

/**
 * Scaling modes supported by graphics backends
 */
typedef enum {
    GFX_SCALE_NEAREST,       // Nearest-neighbor (sharp, pixelated)
    GFX_SCALE_LINEAR,        // Bilinear interpolation (smooth)
    GFX_SCALE_INTEGER,       // Integer scale factor only
    GFX_SCALE_ASPECT,        // Maintain aspect ratio
    GFX_SCALE_FULLSCREEN,    // Stretch to fill screen
} gfx_scaling_mode_t;

/**
 * Backend capabilities flags
 */
typedef enum {
    GFX_CAP_VSYNC           = (1 << 0),  // Hardware vsync support
    GFX_CAP_TRIPLE_BUFFER   = (1 << 1),  // Triple buffering
    GFX_CAP_HARDWARE_ACCEL  = (1 << 2),  // GPU-accelerated scaling
    GFX_CAP_SHADERS         = (1 << 3),  // Shader support
    GFX_CAP_ROTATION        = (1 << 4),  // Screen rotation
    GFX_CAP_OVERLAY         = (1 << 5),  // Hardware overlay planes
} gfx_capabilities_t;

/**
 * Pixel format specification
 */
typedef enum {
    GFX_FORMAT_RGB565,       // 16-bit RGB565
    GFX_FORMAT_BGR565,       // 16-bit BGR565
    GFX_FORMAT_XRGB8888,     // 32-bit XRGB8888
    GFX_FORMAT_ARGB8888,     // 32-bit ARGB8888
} gfx_pixel_format_t;

/**
 * Backend context - opaque pointer for backend-specific state
 */
typedef struct gfx_backend_context gfx_backend_context_t;

/**
 * Graphics backend interface
 *
 * All backends must implement these functions. Platforms register their
 * preferred backend during initialization via gfx_backend_register().
 */
typedef struct gfx_backend {
    /** Backend name for identification and debugging */
    const char* name;

    /** Backend capabilities bitmask */
    uint32_t capabilities;

    /**
     * Initialize the graphics backend.
     *
     * @param width Display width in pixels
     * @param height Display height in pixels
     * @param format Preferred pixel format
     * @return Backend context on success, NULL on failure
     *
     * @note This function is called once at application startup.
     *       It should initialize the display, allocate framebuffers,
     *       and prepare for rendering.
     */
    gfx_backend_context_t* (*init)(int width, int height, gfx_pixel_format_t format);

    /**
     * Cleanup and shutdown the backend.
     *
     * @param ctx Backend context from init()
     *
     * @note Called once at application shutdown. Must release all
     *       resources and restore original display state.
     */
    void (*quit)(gfx_backend_context_t* ctx);

    /**
     * Present a rendered frame to the display.
     *
     * @param ctx Backend context
     * @param buffer Source buffer containing rendered frame
     * @param width Source width in pixels
     * @param height Source height in pixels
     * @param pitch Source pitch in bytes
     * @return 0 on success, negative error code on failure
     *
     * @note CRITICAL PERFORMANCE PATH. This function is called 60 times
     *       per second during gameplay. Optimize for minimal latency.
     *
     *       The buffer contains a fully-rendered frame in the format
     *       specified during init(). The backend should:
     *       1. Scale the buffer to screen dimensions
     *       2. Apply any filters or effects
     *       3. Present to display (with vsync if enabled)
     *       4. Return as quickly as possible
     */
    int (*present)(gfx_backend_context_t* ctx,
                   void* buffer,
                   int width,
                   int height,
                   int pitch);

    /**
     * Set scaling mode.
     *
     * @param ctx Backend context
     * @param mode Desired scaling mode
     * @return 0 on success, negative if mode unsupported
     *
     * @note This affects how present() scales source to display.
     *       Backends should apply the mode immediately or on next
     *       present() call.
     */
    int (*set_scaling)(gfx_backend_context_t* ctx, gfx_scaling_mode_t mode);

    /**
     * Configure vsync behavior.
     *
     * @param ctx Backend context
     * @param enabled Non-zero to enable vsync, zero to disable
     * @return 0 on success, negative if unsupported
     *
     * @note When enabled, present() should block until vblank.
     *       When disabled, present() returns immediately after
     *       queuing the frame.
     */
    int (*set_vsync)(gfx_backend_context_t* ctx, int enabled);

    /**
     * Check if vsync is supported by hardware.
     *
     * @param ctx Backend context
     * @return Non-zero if supported, zero otherwise
     */
    int (*supports_vsync)(gfx_backend_context_t* ctx);

    /**
     * Clear the display to black.
     *
     * @param ctx Backend context
     *
     * @note This should clear all buffers (front and back) to
     *       prevent residual image artifacts.
     */
    void (*clear)(gfx_backend_context_t* ctx);

    /**
     * Get a pointer to the active framebuffer (optional).
     *
     * @param ctx Backend context
     * @param pitch Output parameter for pitch in bytes
     * @return Pointer to framebuffer, or NULL if direct access unsupported
     *
     * @note This allows platforms to directly render into the framebuffer
     *       for maximum performance. Not all backends support this (e.g.,
     *       hardware-accelerated backends may not expose framebuffer).
     */
    void* (*get_framebuffer)(gfx_backend_context_t* ctx, int* pitch);

    /**
     * Set rotation angle (optional).
     *
     * @param ctx Backend context
     * @param angle Rotation in degrees (0, 90, 180, 270)
     * @return 0 on success, negative if unsupported
     *
     * @note Only implement if hardware supports rotation.
     */
    int (*set_rotation)(gfx_backend_context_t* ctx, int angle);

} gfx_backend_t;

/**
 * Backend registration and management
 */

/**
 * Register a graphics backend for use by the system.
 *
 * @param backend Backend implementation
 * @return 0 on success, negative on failure
 *
 * @note Platforms should register their preferred backend(s) during
 *       PLAT_initVideo(). The first registered backend becomes the
 *       default. Multiple backends can be registered to allow runtime
 *       selection based on environment or configuration.
 */
int gfx_backend_register(const gfx_backend_t* backend);

/**
 * Select and initialize a graphics backend.
 *
 * @param name Backend name, or NULL to use default (first registered)
 * @param width Display width
 * @param height Display height
 * @param format Pixel format
 * @return Backend context on success, NULL on failure
 *
 * @note This is called by GFX_init() after backends are registered.
 */
gfx_backend_context_t* gfx_backend_init(const char* name,
                                         int width,
                                         int height,
                                         gfx_pixel_format_t format);

/**
 * Get the currently active backend.
 *
 * @return Pointer to active backend, or NULL if not initialized
 */
const gfx_backend_t* gfx_backend_get_active(void);

/**
 * Get the active backend context.
 *
 * @return Backend context, or NULL if not initialized
 */
gfx_backend_context_t* gfx_backend_get_context(void);

/**
 * Shutdown the active graphics backend.
 */
void gfx_backend_shutdown(void);

/**
 * Built-in backend implementations
 *
 * These are provided by the MinUI core and available to all platforms.
 * Platforms can use these directly or implement custom backends for
 * optimal performance.
 */

/**
 * SDL2 software rendering backend
 *
 * Compatible with all SDL2-enabled platforms. Uses SDL surfaces for
 * rendering and SDL_Flip() for presentation. Suitable for devices where
 * SDL2 performance is acceptable.
 *
 * Capabilities: GFX_CAP_VSYNC (if SDL supports it)
 * Formats: RGB565, XRGB8888
 */
extern const gfx_backend_t gfx_backend_sdl2;

/**
 * SDL2 hardware-accelerated backend
 *
 * Uses SDL2's hardware acceleration features where available. May provide
 * better performance than software rendering on devices with GPU support.
 *
 * Capabilities: GFX_CAP_VSYNC | GFX_CAP_HARDWARE_ACCEL
 * Formats: RGB565, XRGB8888, ARGB8888
 */
extern const gfx_backend_t gfx_backend_sdl2_hw;

/**
 * Linux framebuffer (fbdev) backend
 *
 * Direct framebuffer access via /dev/fb0. Provides maximum performance
 * on Linux systems by bypassing SDL and window system overhead. Requires
 * manual page flipping and scaling implementation.
 *
 * Capabilities: GFX_CAP_TRIPLE_BUFFER
 * Formats: RGB565, BGR565, XRGB8888 (depends on hardware)
 */
extern const gfx_backend_t gfx_backend_fbdev;

/**
 * DRM/KMS backend (future)
 *
 * Modern Linux graphics stack using Direct Rendering Manager and Kernel
 * Mode Setting. Provides low-level GPU access with minimal overhead.
 * Suitable for newer devices with mainline kernel support.
 *
 * Capabilities: GFX_CAP_VSYNC | GFX_CAP_HARDWARE_ACCEL | GFX_CAP_OVERLAY
 * Formats: All formats
 */
extern const gfx_backend_t gfx_backend_drm;

/**
 * Helper macros for backend implementation
 */

/** Define a backend with name and capabilities */
#define GFX_BACKEND_DEFINE(backend_name, caps) \
    .name = #backend_name, \
    .capabilities = caps

/** Check if backend has specific capability */
#define GFX_BACKEND_HAS_CAP(backend, cap) \
    (((backend)->capabilities & (cap)) != 0)

/**
 * Example platform integration:
 *
 * In workspace/<platform>/platform/platform.c:
 *
 * SDL_Surface* PLAT_initVideo(void) {
 *     // Register available backends (in order of preference)
 *     gfx_backend_register(&gfx_backend_fbdev);  // Preferred
 *     gfx_backend_register(&gfx_backend_sdl2);   // Fallback
 *
 *     // Initialize graphics subsystem with best backend
 *     gfx_backend_context_t* ctx = gfx_backend_init(
 *         NULL,  // Use default (first registered)
 *         FIXED_WIDTH,
 *         FIXED_HEIGHT,
 *         GFX_FORMAT_RGB565
 *     );
 *
 *     if (!ctx) {
 *         LOG_error("Failed to initialize graphics backend\n");
 *         return NULL;
 *     }
 *
 *     // Get SDL surface for compatibility with existing code
 *     // (or migrate to use backend directly)
 *     SDL_Surface* screen = SDL_GetVideoSurface();
 *     return screen;
 * }
 *
 * void PLAT_flip(SDL_Surface* screen, int sync) {
 *     const gfx_backend_t* backend = gfx_backend_get_active();
 *     gfx_backend_context_t* ctx = gfx_backend_get_context();
 *
 *     backend->set_vsync(ctx, sync);
 *     backend->present(ctx, screen->pixels,
 *                      screen->w, screen->h, screen->pitch);
 * }
 */

#endif // GFX_BACKEND_H
