#include "gfx_backend.h"
#include "api.h"
#include <stdlib.h>
#include <string.h>

/**
 * Graphics Backend Management
 *
 * This file implements the backend registration and management system,
 * as well as the built-in SDL2 backend implementations.
 */

// Maximum number of registered backends
#define MAX_BACKENDS 8

// Backend registry
static struct {
    const gfx_backend_t* backends[MAX_BACKENDS];
    int count;
    const gfx_backend_t* active;
    gfx_backend_context_t* context;
} gfx_registry = {0};

/**
 * Backend Registration
 */

int gfx_backend_register(const gfx_backend_t* backend) {
    if (!backend) {
        LOG_error("gfx_backend_register: NULL backend\n");
        return -1;
    }

    if (gfx_registry.count >= MAX_BACKENDS) {
        LOG_error("gfx_backend_register: Too many backends (max %d)\n", MAX_BACKENDS);
        return -1;
    }

    // Validate required functions
    if (!backend->init || !backend->quit || !backend->present) {
        LOG_error("gfx_backend_register: Backend '%s' missing required functions\n",
                  backend->name ? backend->name : "<unknown>");
        return -1;
    }

    gfx_registry.backends[gfx_registry.count++] = backend;
    LOG_info("Registered graphics backend: %s (caps=0x%08x)\n",
             backend->name, backend->capabilities);
    return 0;
}

gfx_backend_context_t* gfx_backend_init(const char* name,
                                         int width,
                                         int height,
                                         gfx_pixel_format_t format) {
    const gfx_backend_t* backend = NULL;

    // Find requested backend by name, or use default (first registered)
    if (name) {
        for (int i = 0; i < gfx_registry.count; i++) {
            if (strcmp(gfx_registry.backends[i]->name, name) == 0) {
                backend = gfx_registry.backends[i];
                break;
            }
        }
        if (!backend) {
            LOG_warn("Backend '%s' not found, using default\n", name);
        }
    }

    if (!backend) {
        if (gfx_registry.count == 0) {
            LOG_error("No graphics backends registered\n");
            return NULL;
        }
        backend = gfx_registry.backends[0];
    }

    LOG_info("Initializing graphics backend: %s (%dx%d, format=%d)\n",
             backend->name, width, height, format);

    gfx_backend_context_t* ctx = backend->init(width, height, format);
    if (!ctx) {
        LOG_error("Failed to initialize backend: %s\n", backend->name);
        return NULL;
    }

    gfx_registry.active = backend;
    gfx_registry.context = ctx;

    LOG_info("Graphics backend initialized successfully\n");
    return ctx;
}

const gfx_backend_t* gfx_backend_get_active(void) {
    return gfx_registry.active;
}

gfx_backend_context_t* gfx_backend_get_context(void) {
    return gfx_registry.context;
}

void gfx_backend_shutdown(void) {
    if (gfx_registry.active && gfx_registry.context) {
        LOG_info("Shutting down graphics backend: %s\n", gfx_registry.active->name);
        gfx_registry.active->quit(gfx_registry.context);
        gfx_registry.active = NULL;
        gfx_registry.context = NULL;
    }
}

/**
 * SDL2 Software Backend Implementation
 */

typedef struct {
    SDL_Surface* screen;
    gfx_pixel_format_t format;
    gfx_scaling_mode_t scaling_mode;
    int vsync_enabled;
} sdl2_context_t;

static gfx_backend_context_t* sdl2_init(int width, int height, gfx_pixel_format_t format) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_error("SDL2 backend: SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    // Determine SDL pixel format
    int bpp;
    uint32_t rmask, gmask, bmask, amask;
    switch (format) {
        case GFX_FORMAT_RGB565:
            bpp = 16;
            rmask = 0xF800;
            gmask = 0x07E0;
            bmask = 0x001F;
            amask = 0x0000;
            break;
        case GFX_FORMAT_XRGB8888:
        case GFX_FORMAT_ARGB8888:
            bpp = 32;
            rmask = 0x00FF0000;
            gmask = 0x0000FF00;
            bmask = 0x000000FF;
            amask = (format == GFX_FORMAT_ARGB8888) ? 0xFF000000 : 0x00000000;
            break;
        default:
            LOG_error("SDL2 backend: Unsupported pixel format: %d\n", format);
            return NULL;
    }

    SDL_Surface* screen = SDL_SetVideoMode(width, height, bpp,
                                            SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen) {
        LOG_error("SDL2 backend: SDL_SetVideoMode failed: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    SDL_ShowCursor(0);

    sdl2_context_t* ctx = calloc(1, sizeof(sdl2_context_t));
    if (!ctx) {
        LOG_error("SDL2 backend: Failed to allocate context\n");
        SDL_Quit();
        return NULL;
    }

    ctx->screen = screen;
    ctx->format = format;
    ctx->scaling_mode = GFX_SCALE_ASPECT;
    ctx->vsync_enabled = 1;

    return (gfx_backend_context_t*)ctx;
}

static void sdl2_quit(gfx_backend_context_t* ctx) {
    if (ctx) {
        free(ctx);
    }
    SDL_Quit();
}

static int sdl2_present(gfx_backend_context_t* ctx,
                        void* buffer,
                        int width,
                        int height,
                        int pitch) {
    sdl2_context_t* sdl_ctx = (sdl2_context_t*)ctx;
    if (!sdl_ctx || !sdl_ctx->screen) {
        return -1;
    }

    // If buffer dimensions match screen, direct blit
    if (width == sdl_ctx->screen->w && height == sdl_ctx->screen->h) {
        memcpy(sdl_ctx->screen->pixels, buffer,
               height * pitch);
    } else {
        // Create temporary surface and scale
        SDL_Surface* src = SDL_CreateRGBSurfaceFrom(
            buffer, width, height,
            sdl_ctx->format == GFX_FORMAT_RGB565 ? 16 : 32,
            pitch,
            sdl_ctx->format == GFX_FORMAT_RGB565 ? 0xF800 : 0x00FF0000,
            sdl_ctx->format == GFX_FORMAT_RGB565 ? 0x07E0 : 0x0000FF00,
            sdl_ctx->format == GFX_FORMAT_RGB565 ? 0x001F : 0x000000FF,
            0
        );

        if (src) {
            // Calculate destination rect based on scaling mode
            SDL_Rect dst_rect;
            switch (sdl_ctx->scaling_mode) {
                case GFX_SCALE_FULLSCREEN:
                    dst_rect.x = 0;
                    dst_rect.y = 0;
                    dst_rect.w = sdl_ctx->screen->w;
                    dst_rect.h = sdl_ctx->screen->h;
                    break;

                case GFX_SCALE_ASPECT: {
                    float src_aspect = (float)width / height;
                    float dst_aspect = (float)sdl_ctx->screen->w / sdl_ctx->screen->h;

                    if (src_aspect > dst_aspect) {
                        // Fit to width
                        dst_rect.w = sdl_ctx->screen->w;
                        dst_rect.h = (int)(sdl_ctx->screen->w / src_aspect);
                        dst_rect.x = 0;
                        dst_rect.y = (sdl_ctx->screen->h - dst_rect.h) / 2;
                    } else {
                        // Fit to height
                        dst_rect.h = sdl_ctx->screen->h;
                        dst_rect.w = (int)(sdl_ctx->screen->h * src_aspect);
                        dst_rect.y = 0;
                        dst_rect.x = (sdl_ctx->screen->w - dst_rect.w) / 2;
                    }
                    break;
                }

                case GFX_SCALE_INTEGER: {
                    int scale_x = sdl_ctx->screen->w / width;
                    int scale_y = sdl_ctx->screen->h / height;
                    int scale = (scale_x < scale_y) ? scale_x : scale_y;
                    if (scale < 1) scale = 1;

                    dst_rect.w = width * scale;
                    dst_rect.h = height * scale;
                    dst_rect.x = (sdl_ctx->screen->w - dst_rect.w) / 2;
                    dst_rect.y = (sdl_ctx->screen->h - dst_rect.h) / 2;
                    break;
                }

                default:
                    dst_rect.x = 0;
                    dst_rect.y = 0;
                    dst_rect.w = width;
                    dst_rect.h = height;
                    break;
            }

            SDL_SoftStretch(src, NULL, sdl_ctx->screen, &dst_rect);
            SDL_FreeSurface(src);
        }
    }

    SDL_Flip(sdl_ctx->screen);
    return 0;
}

static int sdl2_set_scaling(gfx_backend_context_t* ctx, gfx_scaling_mode_t mode) {
    sdl2_context_t* sdl_ctx = (sdl2_context_t*)ctx;
    if (!sdl_ctx) return -1;
    sdl_ctx->scaling_mode = mode;
    return 0;
}

static int sdl2_set_vsync(gfx_backend_context_t* ctx, int enabled) {
    sdl2_context_t* sdl_ctx = (sdl2_context_t*)ctx;
    if (!sdl_ctx) return -1;
    sdl_ctx->vsync_enabled = enabled;
    // Note: SDL1.2 doesn't have direct vsync control
    // Actual vsync behavior depends on SDL video driver
    return 0;
}

static int sdl2_supports_vsync(gfx_backend_context_t* ctx) {
    // SDL1.2 may or may not have vsync depending on driver
    // Return 1 optimistically
    return 1;
}

static void sdl2_clear(gfx_backend_context_t* ctx) {
    sdl2_context_t* sdl_ctx = (sdl2_context_t*)ctx;
    if (sdl_ctx && sdl_ctx->screen) {
        SDL_FillRect(sdl_ctx->screen, NULL, 0);
        SDL_Flip(sdl_ctx->screen);
        SDL_FillRect(sdl_ctx->screen, NULL, 0);
    }
}

static void* sdl2_get_framebuffer(gfx_backend_context_t* ctx, int* pitch) {
    sdl2_context_t* sdl_ctx = (sdl2_context_t*)ctx;
    if (sdl_ctx && sdl_ctx->screen) {
        if (pitch) *pitch = sdl_ctx->screen->pitch;
        return sdl_ctx->screen->pixels;
    }
    return NULL;
}

const gfx_backend_t gfx_backend_sdl2 = {
    .name = "sdl2",
    .capabilities = GFX_CAP_VSYNC,
    .init = sdl2_init,
    .quit = sdl2_quit,
    .present = sdl2_present,
    .set_scaling = sdl2_set_scaling,
    .set_vsync = sdl2_set_vsync,
    .supports_vsync = sdl2_supports_vsync,
    .clear = sdl2_clear,
    .get_framebuffer = sdl2_get_framebuffer,
    .set_rotation = NULL,  // Not supported
};

/**
 * Stub implementations for future backends
 */

const gfx_backend_t gfx_backend_sdl2_hw = {
    .name = "sdl2_hw",
    .capabilities = GFX_CAP_VSYNC | GFX_CAP_HARDWARE_ACCEL,
    .init = NULL,  // TODO: Implement hardware-accelerated SDL2 backend
    .quit = NULL,
    .present = NULL,
    .set_scaling = NULL,
    .set_vsync = NULL,
    .supports_vsync = NULL,
    .clear = NULL,
    .get_framebuffer = NULL,
    .set_rotation = NULL,
};

// Framebuffer backend - implemented in gfx_backend_fbdev.c
extern const gfx_backend_t gfx_backend_fbdev_impl;

// Alias for consistency
const gfx_backend_t gfx_backend_fbdev = {
    .name = "fbdev",
    .capabilities = GFX_CAP_TRIPLE_BUFFER,
    .init = NULL,  // Implemented in gfx_backend_fbdev.c
    .quit = NULL,
    .present = NULL,
    .set_scaling = NULL,
    .set_vsync = NULL,
    .supports_vsync = NULL,
    .clear = NULL,
    .get_framebuffer = NULL,
    .set_rotation = NULL,
};

// Note: To use fbdev backend, platforms should register gfx_backend_fbdev_impl
// instead of gfx_backend_fbdev. The implementation is in gfx_backend_fbdev.c
// which can be compiled conditionally for Linux platforms.

const gfx_backend_t gfx_backend_drm = {
    .name = "drm",
    .capabilities = GFX_CAP_VSYNC | GFX_CAP_HARDWARE_ACCEL | GFX_CAP_OVERLAY,
    .init = NULL,  // TODO: Implement DRM/KMS backend
    .quit = NULL,
    .present = NULL,
    .set_scaling = NULL,
    .set_vsync = NULL,
    .supports_vsync = NULL,
    .clear = NULL,
    .get_framebuffer = NULL,
    .set_rotation = NULL,
};
