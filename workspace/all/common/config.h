#ifndef CONFIG_H
#define CONFIG_H

/**
 * Minimal Optional Configuration System
 *
 * This module provides a simple, optional configuration mechanism that
 * respects MinUI's "zero-configuration" philosophy while allowing advanced
 * users and forks to customize behavior.
 *
 * Design Principles:
 * - Default behavior unchanged: Without config file, system works as before
 * - Sensible defaults: All settings have reasonable defaults
 * - No UI complexity: Configuration is file-based, not menu-based
 * - Forward compatible: Unknown settings are ignored
 * - Minimal overhead: Configuration loaded once at startup
 *
 * Configuration File Format:
 * - Plain text key=value pairs
 * - Lines starting with # are comments
 * - Empty lines ignored
 * - Example: /mnt/sdcard/.userdata/minui.conf
 *
 * Example config file:
 *
 *   # MinUI Configuration
 *   # Leave unset to use defaults
 *
 *   # Graphics backend: auto, sdl2, sdl2_hw, fbdev, drm
 *   graphics_backend=auto
 *
 *   # Display scaling: aspect, fullscreen, integer, native
 *   display_scale=aspect
 *
 *   # Sharpness: sharp, crisp, soft
 *   display_sharpness=soft
 *
 *   # Audio latency in milliseconds (32-256)
 *   audio_latency=64
 *
 *   # Save state slots (1-10)
 *   savestate_slots=4
 *
 *   # Frame skip: 0=off, 1=auto, 2-4=fixed
 *   frame_skip=0
 *
 *   # Show FPS counter: 0=off, 1=on
 *   show_fps=0
 *
 *   # Thread video: 0=off, 1=on (if supported)
 *   thread_video=0
 *
 *   # CPU speed overrides: menu, powersave, normal, performance
 *   cpu_speed_menu=600
 *   cpu_speed_normal=1200
 *   cpu_speed_performance=1500
 *
 *   # Custom ROM path (overrides default)
 *   rom_path=/mnt/sdcard/Roms
 *
 *   # Enable debugging: 0=off, 1=on
 *   debug=0
 *
 * Usage in code:
 *
 *   // Load configuration (typically in main())
 *   minui_config_t* config = config_load(NULL);  // NULL = default path
 *
 *   // Use configuration values
 *   if (strcmp(config->graphics_backend, "auto") != 0) {
 *       gfx_backend_init(config->graphics_backend, width, height, format);
 *   }
 *
 *   // Save configuration (optional)
 *   config->display_scale = DISPLAY_SCALE_FULLSCREEN;
 *   config_save(config, NULL);
 *
 *   // Cleanup
 *   config_free(config);
 */

#include <stdint.h>
#include <stddef.h>

// Maximum path length
#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// Display scaling modes
typedef enum {
    DISPLAY_SCALE_ASPECT = 0,
    DISPLAY_SCALE_FULLSCREEN,
    DISPLAY_SCALE_INTEGER,
    DISPLAY_SCALE_NATIVE,
} display_scale_t;

// Sharpness levels
typedef enum {
    DISPLAY_SHARP = 0,
    DISPLAY_CRISP,
    DISPLAY_SOFT,
} display_sharpness_t;

// Frame skip modes
typedef enum {
    FRAME_SKIP_OFF = 0,
    FRAME_SKIP_AUTO,
    FRAME_SKIP_1,
    FRAME_SKIP_2,
    FRAME_SKIP_3,
    FRAME_SKIP_4,
} frame_skip_t;

// Configuration structure
typedef struct {
    // Graphics settings
    char graphics_backend[32];      // "auto", "sdl2", "sdl2_hw", "fbdev", "drm"
    display_scale_t display_scale;
    display_sharpness_t display_sharpness;
    int display_vsync;              // 0=off, 1=lenient, 2=strict

    // Audio settings
    int audio_latency;              // Milliseconds (32-256)
    int audio_sample_rate;          // 0=auto, or specific rate (22050, 44100, etc.)

    // Emulation settings
    int savestate_slots;            // Number of save state slots (1-10)
    frame_skip_t frame_skip;        // Frame skip mode
    int rewind_enabled;             // 0=off, 1=on
    int fast_forward_speed;         // 2-10x (0=unlimited)

    // Performance settings
    int thread_video;               // 0=off, 1=on
    int cpu_speed_menu;             // MHz (0=default)
    int cpu_speed_powersave;        // MHz (0=default)
    int cpu_speed_normal;           // MHz (0=default)
    int cpu_speed_performance;      // MHz (0=default)

    // Paths
    char rom_path[MAX_PATH];        // Custom ROM path
    char bios_path[MAX_PATH];       // Custom BIOS path
    char saves_path[MAX_PATH];      // Custom saves path

    // UI settings
    int show_fps;                   // 0=off, 1=on
    int show_battery;               // 0=off, 1=on
    int menu_timeout;               // Auto-hide timeout in seconds (0=never)

    // Debugging
    int debug;                      // 0=off, 1=on
    int log_level;                  // 0=error, 1=warn, 2=info, 3=debug

} minui_config_t;

/**
 * Load configuration from file.
 *
 * @param path Configuration file path, or NULL for default
 *             (default: /mnt/sdcard/.userdata/minui.conf)
 * @return Configuration structure with defaults for missing values,
 *         or NULL on fatal error (memory allocation failure)
 *
 * @note If file doesn't exist, returns defaults. This is not an error.
 *       All settings have sensible defaults matching current behavior.
 */
minui_config_t* config_load(const char* path);

/**
 * Load configuration with defaults.
 *
 * @return Configuration structure with all default values
 *
 * @note This is equivalent to config_load() when file doesn't exist.
 *       Useful for programmatic initialization.
 */
minui_config_t* config_load_defaults(void);

/**
 * Save configuration to file.
 *
 * @param config Configuration to save
 * @param path Target file path, or NULL for default
 * @return 0 on success, negative on error
 *
 * @note Creates parent directory if it doesn't exist.
 *       Writes all settings, including defaults, to file.
 */
int config_save(const minui_config_t* config, const char* path);

/**
 * Free configuration structure.
 *
 * @param config Configuration to free
 */
void config_free(minui_config_t* config);

/**
 * Get configuration value by key.
 *
 * @param config Configuration structure
 * @param key Configuration key (e.g., "display_scale")
 * @param value Output buffer for value
 * @param value_size Size of output buffer
 * @return 0 on success, negative if key not found
 *
 * @note This is a generic accessor for dynamic lookups.
 *       Direct struct access is preferred for known keys.
 */
int config_get(const minui_config_t* config,
               const char* key,
               char* value,
               size_t value_size);

/**
 * Set configuration value by key.
 *
 * @param config Configuration structure
 * @param key Configuration key
 * @param value New value (string representation)
 * @return 0 on success, negative on error
 *
 * @note This is a generic mutator for dynamic updates.
 *       Direct struct access is preferred for known keys.
 */
int config_set(minui_config_t* config,
               const char* key,
               const char* value);

/**
 * Validate configuration values.
 *
 * @param config Configuration to validate
 * @return 0 if valid, negative if any values are out of range
 *
 * @note This checks all settings and logs warnings for invalid values.
 *       Invalid values are clamped to valid ranges.
 */
int config_validate(minui_config_t* config);

/**
 * Merge configuration from command-line arguments.
 *
 * @param config Configuration to update
 * @param argc Argument count
 * @param argv Argument vector
 * @return Number of arguments consumed
 *
 * @note Recognizes arguments like:
 *       --config-backend=sdl2
 *       --config-scale=fullscreen
 *       --config-debug=1
 */
int config_merge_args(minui_config_t* config, int argc, char** argv);

/**
 * Print configuration to log.
 *
 * @param config Configuration to print
 * @param level Log level (LOG_INFO, LOG_DEBUG, etc.)
 *
 * @note Useful for debugging configuration issues.
 */
void config_print(const minui_config_t* config, int level);

/**
 * Default configuration values
 */
#define CONFIG_DEFAULT_BACKEND          "auto"
#define CONFIG_DEFAULT_SCALE            DISPLAY_SCALE_ASPECT
#define CONFIG_DEFAULT_SHARPNESS        DISPLAY_SOFT
#define CONFIG_DEFAULT_VSYNC            1
#define CONFIG_DEFAULT_AUDIO_LATENCY    64
#define CONFIG_DEFAULT_AUDIO_RATE       0
#define CONFIG_DEFAULT_SAVESTATE_SLOTS  4
#define CONFIG_DEFAULT_FRAME_SKIP       FRAME_SKIP_OFF
#define CONFIG_DEFAULT_REWIND           0
#define CONFIG_DEFAULT_FF_SPEED         3
#define CONFIG_DEFAULT_THREAD_VIDEO     0
#define CONFIG_DEFAULT_SHOW_FPS         0
#define CONFIG_DEFAULT_SHOW_BATTERY     1
#define CONFIG_DEFAULT_MENU_TIMEOUT     0
#define CONFIG_DEFAULT_DEBUG            0
#define CONFIG_DEFAULT_LOG_LEVEL        1

/**
 * Configuration file paths
 */
#define CONFIG_DEFAULT_PATH     "/mnt/sdcard/.userdata/minui.conf"
#define CONFIG_PLATFORM_PATH    "/mnt/sdcard/.userdata/%s/minui.conf"
#define CONFIG_ROM_PATH         "/mnt/sdcard/Roms"
#define CONFIG_BIOS_PATH        "/mnt/sdcard/Bios"
#define CONFIG_SAVES_PATH       "/mnt/sdcard/Saves"

/**
 * Helper macros
 */

// Check if configuration is using default value
#define CONFIG_IS_DEFAULT(cfg, field, default_val) \
    ((cfg)->field == (default_val))

// Get effective value (config or platform-specific)
#define CONFIG_GET_EFFECTIVE(cfg, field, plat_val) \
    (CONFIG_IS_DEFAULT(cfg, field, 0) ? (plat_val) : (cfg)->field)

/**
 * Example integration:
 *
 * In main.c:
 *
 * int main(int argc, char** argv) {
 *     // Load configuration
 *     minui_config_t* config = config_load(NULL);
 *     if (!config) {
 *         LOG_error("Failed to allocate config, using hardcoded defaults\n");
 *         // Proceed with existing hardcoded behavior
 *     }
 *
 *     // Override from command line
 *     config_merge_args(config, argc, argv);
 *
 *     // Print for debugging
 *     if (config->debug) {
 *         config_print(config, LOG_INFO);
 *     }
 *
 *     // Use configuration
 *     if (config->graphics_backend[0] != '\0') {
 *         gfx_backend_register(...);
 *         gfx_backend_init(config->graphics_backend, ...);
 *     }
 *
 *     SND_init(config->audio_sample_rate ? config->audio_sample_rate : 44100,
 *              60.0);
 *
 *     // ... rest of initialization
 *
 *     config_free(config);
 *     return 0;
 * }
 *
 * Gradual migration strategy:
 * 1. Add config system without changing behavior (config_load returns defaults)
 * 2. Platforms opt-in to config by checking values
 * 3. Document configuration options for advanced users
 * 4. No UI changes required - keep zero-config philosophy
 */

#endif // CONFIG_H
