#include "config.h"
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * Configuration System Implementation
 *
 * This file implements the minimal configuration system for MinUI,
 * providing file-based configuration while maintaining zero-config
 * default behavior.
 */

// Helper: Create directory recursively
static int mkdir_recursive(const char* path) {
    char tmp[MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

// Helper: Trim whitespace from string
static char* trim_whitespace(char* str) {
    char* end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

// Helper: Parse integer with bounds checking
static int parse_int(const char* value, int min, int max, int default_val) {
    char* endptr;
    long val = strtol(value, &endptr, 10);

    if (endptr == value || *endptr != '\0') {
        return default_val;
    }

    if (val < min) return min;
    if (val > max) return max;
    return (int)val;
}

// Helper: Parse boolean
static int parse_bool(const char* value) {
    if (strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "on") == 0 ||
        strcmp(value, "1") == 0) {
        return 1;
    }
    return 0;
}

// Helper: Parse display scale enum
static display_scale_t parse_display_scale(const char* value) {
    if (strcasecmp(value, "aspect") == 0) return DISPLAY_SCALE_ASPECT;
    if (strcasecmp(value, "fullscreen") == 0) return DISPLAY_SCALE_FULLSCREEN;
    if (strcasecmp(value, "integer") == 0) return DISPLAY_SCALE_INTEGER;
    if (strcasecmp(value, "native") == 0) return DISPLAY_SCALE_NATIVE;
    return CONFIG_DEFAULT_SCALE;
}

// Helper: Parse sharpness enum
static display_sharpness_t parse_sharpness(const char* value) {
    if (strcasecmp(value, "sharp") == 0) return DISPLAY_SHARP;
    if (strcasecmp(value, "crisp") == 0) return DISPLAY_CRISP;
    if (strcasecmp(value, "soft") == 0) return DISPLAY_SOFT;
    return CONFIG_DEFAULT_SHARPNESS;
}

// Helper: Parse frame skip enum
static frame_skip_t parse_frame_skip(const char* value) {
    if (strcasecmp(value, "off") == 0 || strcmp(value, "0") == 0) return FRAME_SKIP_OFF;
    if (strcasecmp(value, "auto") == 0 || strcmp(value, "1") == 0) return FRAME_SKIP_AUTO;
    if (strcmp(value, "2") == 0) return FRAME_SKIP_1;
    if (strcmp(value, "3") == 0) return FRAME_SKIP_2;
    if (strcmp(value, "4") == 0) return FRAME_SKIP_3;
    return CONFIG_DEFAULT_FRAME_SKIP;
}

/**
 * Load configuration with defaults
 */
minui_config_t* config_load_defaults(void) {
    minui_config_t* config = calloc(1, sizeof(minui_config_t));
    if (!config) {
        LOG_error("config_load_defaults: Failed to allocate config\n");
        return NULL;
    }

    // Graphics settings
    strncpy(config->graphics_backend, CONFIG_DEFAULT_BACKEND, sizeof(config->graphics_backend) - 1);
    config->display_scale = CONFIG_DEFAULT_SCALE;
    config->display_sharpness = CONFIG_DEFAULT_SHARPNESS;
    config->display_vsync = CONFIG_DEFAULT_VSYNC;

    // Audio settings
    config->audio_latency = CONFIG_DEFAULT_AUDIO_LATENCY;
    config->audio_sample_rate = CONFIG_DEFAULT_AUDIO_RATE;

    // Emulation settings
    config->savestate_slots = CONFIG_DEFAULT_SAVESTATE_SLOTS;
    config->frame_skip = CONFIG_DEFAULT_FRAME_SKIP;
    config->rewind_enabled = CONFIG_DEFAULT_REWIND;
    config->fast_forward_speed = CONFIG_DEFAULT_FF_SPEED;

    // Performance settings
    config->thread_video = CONFIG_DEFAULT_THREAD_VIDEO;
    config->cpu_speed_menu = 0;
    config->cpu_speed_powersave = 0;
    config->cpu_speed_normal = 0;
    config->cpu_speed_performance = 0;

    // Paths (use defaults from platform)
    strncpy(config->rom_path, CONFIG_ROM_PATH, sizeof(config->rom_path) - 1);
    strncpy(config->bios_path, CONFIG_BIOS_PATH, sizeof(config->bios_path) - 1);
    strncpy(config->saves_path, CONFIG_SAVES_PATH, sizeof(config->saves_path) - 1);

    // UI settings
    config->show_fps = CONFIG_DEFAULT_SHOW_FPS;
    config->show_battery = CONFIG_DEFAULT_SHOW_BATTERY;
    config->menu_timeout = CONFIG_DEFAULT_MENU_TIMEOUT;

    // Debugging
    config->debug = CONFIG_DEFAULT_DEBUG;
    config->log_level = CONFIG_DEFAULT_LOG_LEVEL;

    return config;
}

/**
 * Parse single configuration line
 */
static void parse_config_line(minui_config_t* config, const char* line) {
    char key[128];
    char value[256];
    const char* equals = strchr(line, '=');

    if (!equals) return;

    // Extract key and value
    size_t key_len = equals - line;
    if (key_len >= sizeof(key)) return;

    strncpy(key, line, key_len);
    key[key_len] = '\0';
    char* trimmed_key = trim_whitespace(key);

    strncpy(value, equals + 1, sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';
    char* trimmed_value = trim_whitespace(value);

    // Parse based on key
    if (strcmp(trimmed_key, "graphics_backend") == 0) {
        strncpy(config->graphics_backend, trimmed_value, sizeof(config->graphics_backend) - 1);
    }
    else if (strcmp(trimmed_key, "display_scale") == 0) {
        config->display_scale = parse_display_scale(trimmed_value);
    }
    else if (strcmp(trimmed_key, "display_sharpness") == 0) {
        config->display_sharpness = parse_sharpness(trimmed_value);
    }
    else if (strcmp(trimmed_key, "display_vsync") == 0) {
        config->display_vsync = parse_int(trimmed_value, 0, 2, CONFIG_DEFAULT_VSYNC);
    }
    else if (strcmp(trimmed_key, "audio_latency") == 0) {
        config->audio_latency = parse_int(trimmed_value, 32, 256, CONFIG_DEFAULT_AUDIO_LATENCY);
    }
    else if (strcmp(trimmed_key, "audio_sample_rate") == 0) {
        config->audio_sample_rate = parse_int(trimmed_value, 0, 192000, 0);
    }
    else if (strcmp(trimmed_key, "savestate_slots") == 0) {
        config->savestate_slots = parse_int(trimmed_value, 1, 10, CONFIG_DEFAULT_SAVESTATE_SLOTS);
    }
    else if (strcmp(trimmed_key, "frame_skip") == 0) {
        config->frame_skip = parse_frame_skip(trimmed_value);
    }
    else if (strcmp(trimmed_key, "rewind_enabled") == 0) {
        config->rewind_enabled = parse_bool(trimmed_value);
    }
    else if (strcmp(trimmed_key, "fast_forward_speed") == 0) {
        config->fast_forward_speed = parse_int(trimmed_value, 0, 10, CONFIG_DEFAULT_FF_SPEED);
    }
    else if (strcmp(trimmed_key, "thread_video") == 0) {
        config->thread_video = parse_bool(trimmed_value);
    }
    else if (strcmp(trimmed_key, "cpu_speed_menu") == 0) {
        config->cpu_speed_menu = parse_int(trimmed_value, 0, 10000, 0);
    }
    else if (strcmp(trimmed_key, "cpu_speed_powersave") == 0) {
        config->cpu_speed_powersave = parse_int(trimmed_value, 0, 10000, 0);
    }
    else if (strcmp(trimmed_key, "cpu_speed_normal") == 0) {
        config->cpu_speed_normal = parse_int(trimmed_value, 0, 10000, 0);
    }
    else if (strcmp(trimmed_key, "cpu_speed_performance") == 0) {
        config->cpu_speed_performance = parse_int(trimmed_value, 0, 10000, 0);
    }
    else if (strcmp(trimmed_key, "rom_path") == 0) {
        strncpy(config->rom_path, trimmed_value, sizeof(config->rom_path) - 1);
    }
    else if (strcmp(trimmed_key, "bios_path") == 0) {
        strncpy(config->bios_path, trimmed_value, sizeof(config->bios_path) - 1);
    }
    else if (strcmp(trimmed_key, "saves_path") == 0) {
        strncpy(config->saves_path, trimmed_value, sizeof(config->saves_path) - 1);
    }
    else if (strcmp(trimmed_key, "show_fps") == 0) {
        config->show_fps = parse_bool(trimmed_value);
    }
    else if (strcmp(trimmed_key, "show_battery") == 0) {
        config->show_battery = parse_bool(trimmed_value);
    }
    else if (strcmp(trimmed_key, "menu_timeout") == 0) {
        config->menu_timeout = parse_int(trimmed_value, 0, 300, 0);
    }
    else if (strcmp(trimmed_key, "debug") == 0) {
        config->debug = parse_bool(trimmed_value);
    }
    else if (strcmp(trimmed_key, "log_level") == 0) {
        config->log_level = parse_int(trimmed_value, 0, 3, CONFIG_DEFAULT_LOG_LEVEL);
    }
    else {
        LOG_debug("config: Unknown key '%s'\n", trimmed_key);
    }
}

/**
 * Load configuration from file
 */
minui_config_t* config_load(const char* path) {
    // Start with defaults
    minui_config_t* config = config_load_defaults();
    if (!config) {
        return NULL;
    }

    // Determine config file path
    const char* config_path = path ? path : CONFIG_DEFAULT_PATH;

    FILE* fp = fopen(config_path, "r");
    if (!fp) {
        // File doesn't exist - not an error, use defaults
        LOG_info("Config file not found at '%s', using defaults\n", config_path);
        return config;
    }

    LOG_info("Loading configuration from '%s'\n", config_path);

    char line[512];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Trim whitespace
        char* trimmed = trim_whitespace(line);

        // Skip empty lines and comments
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        // Parse the line
        parse_config_line(config, trimmed);
    }

    fclose(fp);

    // Validate configuration
    config_validate(config);

    LOG_info("Configuration loaded successfully\n");
    return config;
}

/**
 * Save configuration to file
 */
int config_save(const minui_config_t* config, const char* path) {
    if (!config) {
        return -1;
    }

    const char* config_path = path ? path : CONFIG_DEFAULT_PATH;

    // Create directory if it doesn't exist
    char dir_path[MAX_PATH];
    strncpy(dir_path, config_path, sizeof(dir_path) - 1);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (mkdir_recursive(dir_path) != 0) {
            LOG_error("config_save: Failed to create directory '%s'\n", dir_path);
            return -1;
        }
    }

    FILE* fp = fopen(config_path, "w");
    if (!fp) {
        LOG_error("config_save: Failed to open '%s' for writing: %s\n",
                  config_path, strerror(errno));
        return -1;
    }

    fprintf(fp, "# MinUI Configuration\n");
    fprintf(fp, "# Automatically generated - edit carefully\n");
    fprintf(fp, "# Leave unset to use defaults\n\n");

    fprintf(fp, "# Graphics backend: auto, sdl2, sdl2_hw, fbdev, drm\n");
    fprintf(fp, "graphics_backend=%s\n\n", config->graphics_backend);

    fprintf(fp, "# Display scaling: aspect, fullscreen, integer, native\n");
    const char* scale_names[] = {"aspect", "fullscreen", "integer", "native"};
    fprintf(fp, "display_scale=%s\n", scale_names[config->display_scale]);

    fprintf(fp, "# Sharpness: sharp, crisp, soft\n");
    const char* sharp_names[] = {"sharp", "crisp", "soft"};
    fprintf(fp, "display_sharpness=%s\n", sharp_names[config->display_sharpness]);

    fprintf(fp, "# VSync: 0=off, 1=lenient, 2=strict\n");
    fprintf(fp, "display_vsync=%d\n\n", config->display_vsync);

    fprintf(fp, "# Audio latency in milliseconds (32-256)\n");
    fprintf(fp, "audio_latency=%d\n", config->audio_latency);

    fprintf(fp, "# Audio sample rate (0=auto)\n");
    fprintf(fp, "audio_sample_rate=%d\n\n", config->audio_sample_rate);

    fprintf(fp, "# Save state slots (1-10)\n");
    fprintf(fp, "savestate_slots=%d\n", config->savestate_slots);

    fprintf(fp, "# Frame skip: 0=off, 1=auto, 2-4=fixed\n");
    fprintf(fp, "frame_skip=%d\n", config->frame_skip);

    fprintf(fp, "# Rewind enabled: 0=off, 1=on\n");
    fprintf(fp, "rewind_enabled=%d\n", config->rewind_enabled);

    fprintf(fp, "# Fast forward speed (0=unlimited, 2-10=multiplier)\n");
    fprintf(fp, "fast_forward_speed=%d\n\n", config->fast_forward_speed);

    fprintf(fp, "# Thread video: 0=off, 1=on (if supported)\n");
    fprintf(fp, "thread_video=%d\n\n", config->thread_video);

    fprintf(fp, "# CPU speed overrides in MHz (0=use default)\n");
    fprintf(fp, "cpu_speed_menu=%d\n", config->cpu_speed_menu);
    fprintf(fp, "cpu_speed_powersave=%d\n", config->cpu_speed_powersave);
    fprintf(fp, "cpu_speed_normal=%d\n", config->cpu_speed_normal);
    fprintf(fp, "cpu_speed_performance=%d\n\n", config->cpu_speed_performance);

    fprintf(fp, "# Custom paths\n");
    fprintf(fp, "rom_path=%s\n", config->rom_path);
    fprintf(fp, "bios_path=%s\n", config->bios_path);
    fprintf(fp, "saves_path=%s\n\n", config->saves_path);

    fprintf(fp, "# UI settings\n");
    fprintf(fp, "show_fps=%d\n", config->show_fps);
    fprintf(fp, "show_battery=%d\n", config->show_battery);
    fprintf(fp, "menu_timeout=%d\n\n", config->menu_timeout);

    fprintf(fp, "# Debugging\n");
    fprintf(fp, "debug=%d\n", config->debug);
    fprintf(fp, "log_level=%d\n", config->log_level);

    fclose(fp);

    LOG_info("Configuration saved to '%s'\n", config_path);
    return 0;
}

/**
 * Free configuration
 */
void config_free(minui_config_t* config) {
    free(config);
}

/**
 * Validate configuration
 */
int config_validate(minui_config_t* config) {
    if (!config) return -1;

    int errors = 0;

    // Validate audio latency
    if (config->audio_latency < 32 || config->audio_latency > 256) {
        LOG_warn("config_validate: audio_latency %d out of range [32, 256], clamping\n",
                 config->audio_latency);
        config->audio_latency = config->audio_latency < 32 ? 32 : 256;
        errors++;
    }

    // Validate savestate slots
    if (config->savestate_slots < 1 || config->savestate_slots > 10) {
        LOG_warn("config_validate: savestate_slots %d out of range [1, 10], clamping\n",
                 config->savestate_slots);
        config->savestate_slots = config->savestate_slots < 1 ? 1 : 10;
        errors++;
    }

    // Validate fast forward speed
    if (config->fast_forward_speed < 0 || config->fast_forward_speed > 10) {
        LOG_warn("config_validate: fast_forward_speed %d out of range [0, 10], clamping\n",
                 config->fast_forward_speed);
        config->fast_forward_speed = config->fast_forward_speed < 0 ? 0 : 10;
        errors++;
    }

    return errors > 0 ? -1 : 0;
}

/**
 * Get configuration value by key
 */
int config_get(const minui_config_t* config, const char* key, char* value, size_t value_size) {
    if (!config || !key || !value || value_size == 0) {
        return -1;
    }

    // Simple string matching for dynamic access
    if (strcmp(key, "graphics_backend") == 0) {
        snprintf(value, value_size, "%s", config->graphics_backend);
        return 0;
    }
    if (strcmp(key, "thread_video") == 0) {
        snprintf(value, value_size, "%d", config->thread_video);
        return 0;
    }
    // Add more as needed...

    return -1;
}

/**
 * Set configuration value by key
 */
int config_set(minui_config_t* config, const char* key, const char* value) {
    if (!config || !key || !value) {
        return -1;
    }

    // Create a temporary line and parse it
    char line[512];
    snprintf(line, sizeof(line), "%s=%s", key, value);
    parse_config_line(config, line);

    return 0;
}

/**
 * Merge command-line arguments
 */
int config_merge_args(minui_config_t* config, int argc, char** argv) {
    if (!config) return 0;

    int consumed = 0;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--config-", 9) == 0) {
            const char* arg = argv[i] + 9;
            const char* equals = strchr(arg, '=');

            if (equals) {
                char key[128];
                size_t key_len = equals - arg;
                if (key_len < sizeof(key)) {
                    strncpy(key, arg, key_len);
                    key[key_len] = '\0';
                    config_set(config, key, equals + 1);
                    consumed++;
                }
            }
        }
    }

    if (consumed > 0) {
        LOG_info("Merged %d command-line arguments into config\n", consumed);
        config_validate(config);
    }

    return consumed;
}

/**
 * Print configuration
 */
void config_print(const minui_config_t* config, int level) {
    if (!config) return;

    LOG_note(level, "=== MinUI Configuration ===\n");
    LOG_note(level, "Graphics backend: %s\n", config->graphics_backend);
    LOG_note(level, "Display scale: %d\n", config->display_scale);
    LOG_note(level, "Display sharpness: %d\n", config->display_sharpness);
    LOG_note(level, "Display vsync: %d\n", config->display_vsync);
    LOG_note(level, "Audio latency: %d ms\n", config->audio_latency);
    LOG_note(level, "Audio sample rate: %d Hz\n", config->audio_sample_rate);
    LOG_note(level, "Savestate slots: %d\n", config->savestate_slots);
    LOG_note(level, "Frame skip: %d\n", config->frame_skip);
    LOG_note(level, "Thread video: %d\n", config->thread_video);
    LOG_note(level, "Show FPS: %d\n", config->show_fps);
    LOG_note(level, "Debug: %d\n", config->debug);
    LOG_note(level, "===========================\n");
}
