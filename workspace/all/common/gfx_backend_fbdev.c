#include "gfx_backend.h"
#include "api.h"
#include "scaler.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

/**
 * Linux Framebuffer (fbdev) Graphics Backend
 *
 * This backend provides direct framebuffer access via /dev/fb0 for maximum
 * performance on Linux systems. It bypasses SDL and window system overhead,
 * making it ideal for embedded devices where SDL2 performance is insufficient.
 *
 * Features:
 * - Direct framebuffer access for minimal latency
 * - Double/triple buffering via page flipping
 * - Manual scaling and format conversion
 * - Support for RGB565, BGR565, and XRGB8888
 *
 * Limitations:
 * - Linux-specific (requires fbdev driver)
 * - No GPU acceleration (software rendering only)
 * - Manual vsync via ioctl (if supported by driver)
 */

typedef struct {
    // Framebuffer device
    int fd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    // Display properties
    int width;
    int height;
    int bytes_per_pixel;
    gfx_pixel_format_t format;

    // Framebuffer mapping
    uint8_t* framebuffer;
    size_t framebuffer_size;

    // Page flipping (triple buffering)
    int num_buffers;
    int current_buffer;
    uint8_t* buffers[3];

    // Scaling
    gfx_scaling_mode_t scaling_mode;
    scaler_t scaler;
    uint8_t* scaled_buffer;
    int scaled_width;
    int scaled_height;

    // Vsync
    int vsync_enabled;

} fbdev_context_t;

/**
 * Detect pixel format from framebuffer info
 */
static gfx_pixel_format_t detect_format(struct fb_var_screeninfo* vinfo) {
    if (vinfo->bits_per_pixel == 16) {
        // Check if RGB565 or BGR565
        if (vinfo->red.offset == 11) {
            return GFX_FORMAT_RGB565;
        } else {
            return GFX_FORMAT_BGR565;
        }
    } else if (vinfo->bits_per_pixel == 32) {
        return GFX_FORMAT_XRGB8888;
    }

    LOG_warn("fbdev: Unknown pixel format (bpp=%d), assuming RGB565\n",
             vinfo->bits_per_pixel);
    return GFX_FORMAT_RGB565;
}

/**
 * Calculate bytes per pixel from format
 */
static int format_bpp(gfx_pixel_format_t format) {
    switch (format) {
        case GFX_FORMAT_RGB565:
        case GFX_FORMAT_BGR565:
            return 2;
        case GFX_FORMAT_XRGB8888:
        case GFX_FORMAT_ARGB8888:
            return 4;
        default:
            return 2;
    }
}

/**
 * Initialize framebuffer backend
 */
static gfx_backend_context_t* fbdev_init(int width, int height, gfx_pixel_format_t format) {
    fbdev_context_t* ctx = calloc(1, sizeof(fbdev_context_t));
    if (!ctx) {
        LOG_error("fbdev_init: Failed to allocate context\n");
        return NULL;
    }

    // Open framebuffer device
    ctx->fd = open("/dev/fb0", O_RDWR);
    if (ctx->fd < 0) {
        LOG_error("fbdev_init: Failed to open /dev/fb0: %s\n", strerror(errno));
        free(ctx);
        return NULL;
    }

    // Get variable screen info
    if (ioctl(ctx->fd, FBIOGET_VSCREENINFO, &ctx->vinfo) < 0) {
        LOG_error("fbdev_init: FBIOGET_VSCREENINFO failed: %s\n", strerror(errno));
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Get fixed screen info
    if (ioctl(ctx->fd, FBIOGET_FSCREENINFO, &ctx->finfo) < 0) {
        LOG_error("fbdev_init: FBIOGET_FSCREENINFO failed: %s\n", strerror(errno));
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Store display properties
    ctx->width = ctx->vinfo.xres;
    ctx->height = ctx->vinfo.yres;
    ctx->format = detect_format(&ctx->vinfo);
    ctx->bytes_per_pixel = ctx->vinfo.bits_per_pixel / 8;

    LOG_info("fbdev_init: Display %dx%d, %d bpp, format=%d\n",
             ctx->width, ctx->height, ctx->vinfo.bits_per_pixel, ctx->format);

    // Calculate framebuffer size
    ctx->framebuffer_size = ctx->finfo.smem_len;

    // Map framebuffer to memory
    ctx->framebuffer = mmap(NULL, ctx->framebuffer_size,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            ctx->fd, 0);
    if (ctx->framebuffer == MAP_FAILED) {
        LOG_error("fbdev_init: mmap failed: %s\n", strerror(errno));
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    // Setup triple buffering if supported
    ctx->num_buffers = 1;
    if (ctx->vinfo.yres_virtual >= ctx->vinfo.yres * 3) {
        ctx->num_buffers = 3;
        LOG_info("fbdev_init: Triple buffering enabled\n");
    } else if (ctx->vinfo.yres_virtual >= ctx->vinfo.yres * 2) {
        ctx->num_buffers = 2;
        LOG_info("fbdev_init: Double buffering enabled\n");
    }

    // Setup buffer pointers
    size_t buffer_size = ctx->width * ctx->height * ctx->bytes_per_pixel;
    for (int i = 0; i < ctx->num_buffers; i++) {
        ctx->buffers[i] = ctx->framebuffer + (i * buffer_size);
    }
    ctx->current_buffer = 0;

    // Allocate scaling buffer
    ctx->scaled_buffer = malloc(buffer_size);
    if (!ctx->scaled_buffer) {
        LOG_error("fbdev_init: Failed to allocate scaling buffer\n");
        munmap(ctx->framebuffer, ctx->framebuffer_size);
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->scaling_mode = GFX_SCALE_ASPECT;
    ctx->scaler = scale2x_c16; // Default scaler
    ctx->vsync_enabled = 1;

    LOG_info("fbdev_init: Initialization successful\n");
    return (gfx_backend_context_t*)ctx;
}

/**
 * Cleanup framebuffer backend
 */
static void fbdev_quit(gfx_backend_context_t* ctx) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return;

    LOG_info("fbdev_quit: Shutting down\n");

    if (fbdev_ctx->scaled_buffer) {
        free(fbdev_ctx->scaled_buffer);
    }

    if (fbdev_ctx->framebuffer != MAP_FAILED) {
        munmap(fbdev_ctx->framebuffer, fbdev_ctx->framebuffer_size);
    }

    if (fbdev_ctx->fd >= 0) {
        close(fbdev_ctx->fd);
    }

    free(fbdev_ctx);
}

/**
 * Wait for vsync
 */
static void fbdev_wait_vsync(fbdev_context_t* ctx) {
    if (!ctx->vsync_enabled) return;

    int dummy = 0;
    if (ioctl(ctx->fd, FBIO_WAITFORVSYNC, &dummy) < 0) {
        // Vsync not supported, fallback to sleep
        usleep(16666); // ~60Hz
    }
}

/**
 * Flip to next buffer
 */
static void fbdev_flip_buffer(fbdev_context_t* ctx) {
    if (ctx->num_buffers <= 1) {
        // No page flipping, just wait for vsync
        fbdev_wait_vsync(ctx);
        return;
    }

    // Pan display to next buffer
    ctx->current_buffer = (ctx->current_buffer + 1) % ctx->num_buffers;

    struct fb_var_screeninfo vinfo = ctx->vinfo;
    vinfo.yoffset = ctx->height * ctx->current_buffer;

    if (ioctl(ctx->fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
        LOG_warn("fbdev_flip_buffer: FBIOPAN_DISPLAY failed: %s\n", strerror(errno));
    }

    fbdev_wait_vsync(ctx);
}

/**
 * Calculate destination rectangle for scaling mode
 */
static void calculate_dst_rect(fbdev_context_t* ctx,
                                int src_width, int src_height,
                                int* dst_x, int* dst_y,
                                int* dst_w, int* dst_h) {
    switch (ctx->scaling_mode) {
        case GFX_SCALE_FULLSCREEN:
            *dst_x = 0;
            *dst_y = 0;
            *dst_w = ctx->width;
            *dst_h = ctx->height;
            break;

        case GFX_SCALE_ASPECT: {
            float src_aspect = (float)src_width / src_height;
            float dst_aspect = (float)ctx->width / ctx->height;

            if (src_aspect > dst_aspect) {
                // Fit to width
                *dst_w = ctx->width;
                *dst_h = (int)(ctx->width / src_aspect);
                *dst_x = 0;
                *dst_y = (ctx->height - *dst_h) / 2;
            } else {
                // Fit to height
                *dst_h = ctx->height;
                *dst_w = (int)(ctx->height * src_aspect);
                *dst_y = 0;
                *dst_x = (ctx->width - *dst_w) / 2;
            }
            break;
        }

        case GFX_SCALE_INTEGER: {
            int scale_x = ctx->width / src_width;
            int scale_y = ctx->height / src_height;
            int scale = (scale_x < scale_y) ? scale_x : scale_y;
            if (scale < 1) scale = 1;

            *dst_w = src_width * scale;
            *dst_h = src_height * scale;
            *dst_x = (ctx->width - *dst_w) / 2;
            *dst_y = (ctx->height - *dst_h) / 2;
            break;
        }

        case GFX_SCALE_NEAREST:
        default:
            *dst_x = (ctx->width - src_width) / 2;
            *dst_y = (ctx->height - src_height) / 2;
            *dst_w = src_width;
            *dst_h = src_height;
            break;
    }
}

/**
 * Present frame to display
 */
static int fbdev_present(gfx_backend_context_t* ctx,
                         void* buffer,
                         int width,
                         int height,
                         int pitch) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx || !buffer) {
        return -1;
    }

    // Get current target buffer
    uint8_t* target = fbdev_ctx->buffers[fbdev_ctx->current_buffer];

    // Calculate destination rectangle
    int dst_x, dst_y, dst_w, dst_h;
    calculate_dst_rect(fbdev_ctx, width, height, &dst_x, &dst_y, &dst_w, &dst_h);

    // If source matches destination exactly, direct copy
    if (width == dst_w && height == dst_h) {
        int line_bytes = width * fbdev_ctx->bytes_per_pixel;
        for (int y = 0; y < height && (dst_y + y) < fbdev_ctx->height; y++) {
            uint8_t* src_line = (uint8_t*)buffer + (y * pitch);
            uint8_t* dst_line = target +
                ((dst_y + y) * fbdev_ctx->width + dst_x) * fbdev_ctx->bytes_per_pixel;

            if (dst_x >= 0 && (dst_x + width) <= fbdev_ctx->width) {
                memcpy(dst_line, src_line, line_bytes);
            }
        }
    } else {
        // Use software scaler
        // Note: This is a simplified implementation
        // A production version would use optimized scalers from scaler.c

        // For now, use nearest-neighbor scaling
        float x_ratio = (float)width / dst_w;
        float y_ratio = (float)height / dst_h;

        for (int y = 0; y < dst_h && (dst_y + y) < fbdev_ctx->height; y++) {
            int src_y = (int)(y * y_ratio);
            if (src_y >= height) continue;

            for (int x = 0; x < dst_w && (dst_x + x) < fbdev_ctx->width; x++) {
                int src_x = (int)(x * x_ratio);
                if (src_x >= width) continue;

                uint8_t* src_pixel = (uint8_t*)buffer +
                    (src_y * pitch) + (src_x * fbdev_ctx->bytes_per_pixel);
                uint8_t* dst_pixel = target +
                    ((dst_y + y) * fbdev_ctx->width + (dst_x + x)) * fbdev_ctx->bytes_per_pixel;

                memcpy(dst_pixel, src_pixel, fbdev_ctx->bytes_per_pixel);
            }
        }
    }

    // Flip to display
    fbdev_flip_buffer(fbdev_ctx);

    return 0;
}

/**
 * Set scaling mode
 */
static int fbdev_set_scaling(gfx_backend_context_t* ctx, gfx_scaling_mode_t mode) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return -1;

    fbdev_ctx->scaling_mode = mode;
    LOG_info("fbdev_set_scaling: mode=%d\n", mode);
    return 0;
}

/**
 * Set vsync
 */
static int fbdev_set_vsync(gfx_backend_context_t* ctx, int enabled) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return -1;

    fbdev_ctx->vsync_enabled = enabled;
    LOG_info("fbdev_set_vsync: enabled=%d\n", enabled);
    return 0;
}

/**
 * Check vsync support
 */
static int fbdev_supports_vsync(gfx_backend_context_t* ctx) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return 0;

    // Try FBIO_WAITFORVSYNC to see if it's supported
    int dummy = 0;
    int ret = ioctl(fbdev_ctx->fd, FBIO_WAITFORVSYNC, &dummy);
    return (ret == 0) ? 1 : 0;
}

/**
 * Clear display
 */
static void fbdev_clear(gfx_backend_context_t* ctx) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return;

    // Clear all buffers
    for (int i = 0; i < fbdev_ctx->num_buffers; i++) {
        memset(fbdev_ctx->buffers[i],
               0,
               fbdev_ctx->width * fbdev_ctx->height * fbdev_ctx->bytes_per_pixel);
    }

    // Flip to show cleared buffer
    fbdev_flip_buffer(fbdev_ctx);
}

/**
 * Get framebuffer pointer
 */
static void* fbdev_get_framebuffer(gfx_backend_context_t* ctx, int* pitch) {
    fbdev_context_t* fbdev_ctx = (fbdev_context_t*)ctx;
    if (!fbdev_ctx) return NULL;

    if (pitch) {
        *pitch = fbdev_ctx->width * fbdev_ctx->bytes_per_pixel;
    }

    return fbdev_ctx->buffers[fbdev_ctx->current_buffer];
}

/**
 * Framebuffer backend definition
 */
const gfx_backend_t gfx_backend_fbdev_impl = {
    .name = "fbdev",
    .capabilities = GFX_CAP_TRIPLE_BUFFER,
    .init = fbdev_init,
    .quit = fbdev_quit,
    .present = fbdev_present,
    .set_scaling = fbdev_set_scaling,
    .set_vsync = fbdev_set_vsync,
    .supports_vsync = fbdev_supports_vsync,
    .clear = fbdev_clear,
    .get_framebuffer = fbdev_get_framebuffer,
    .set_rotation = NULL,  // Not implemented
};

// Update the stub in gfx_backend.c to point to this implementation
// This allows platforms to use the fbdev backend when needed
