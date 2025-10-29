#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

// Platform configuration for dev platform
// Allows simulating different device profiles

typedef enum {
    WINDOW_MODE_WINDOWED,
    WINDOW_MODE_FULLSCREEN
} window_mode_t;

typedef enum {
    PIXEL_FORMAT_RGB565,    // 16-bit
    PIXEL_FORMAT_ARGB8888   // 32-bit
} pixel_format_t;

typedef struct {
    char name[64];
    char description[256];
    int screen_width;
    int screen_height;
    int scale;
    int bpp;  // bits per pixel (16 or 32)
} device_profile_t;

typedef struct {
    char active_profile[64];
    window_mode_t window_mode;
    int vsync;
    pixel_format_t pixel_format;
    device_profile_t profile;
} platform_config_t;

// Load platform configuration from file
// Returns 0 on success, -1 on error
int platform_config_load(const char* config_path, platform_config_t* config);

// Load defaults
void platform_config_defaults(platform_config_t* config);

// Get config file path (looks in current dir, then ~/.minui/)
const char* platform_config_get_path(void);

#endif // PLATFORM_CONFIG_H
