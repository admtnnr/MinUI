// Platform configuration parser for dev platform

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "platform_config.h"

// Trim whitespace from string
static char* trim(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

// Parse a section name from [section]
static int parse_section(const char* line, char* section, size_t section_size) {
    const char* start = strchr(line, '[');
    const char* end = strchr(line, ']');

    if (!start || !end || end <= start) return 0;

    size_t len = end - start - 1;
    if (len >= section_size) len = section_size - 1;

    strncpy(section, start + 1, len);
    section[len] = '\0';

    return 1;
}

// Parse key=value pair
static int parse_keyvalue(const char* line, char* key, char* value, size_t size) {
    const char* equals = strchr(line, '=');
    if (!equals) return 0;

    // Extract key
    size_t key_len = equals - line;
    if (key_len >= size) key_len = size - 1;
    strncpy(key, line, key_len);
    key[key_len] = '\0';

    // Trim key
    char* trimmed_key = trim(key);
    if (trimmed_key != key) {
        memmove(key, trimmed_key, strlen(trimmed_key) + 1);
    }

    // Extract value
    strncpy(value, equals + 1, size - 1);
    value[size - 1] = '\0';

    // Trim value
    char* trimmed_value = trim(value);
    if (trimmed_value != value) {
        memmove(value, trimmed_value, strlen(trimmed_value) + 1);
    }

    return 1;
}

void platform_config_defaults(platform_config_t* config) {
    strcpy(config->active_profile, "rg35xx");
    config->window_mode = WINDOW_MODE_WINDOWED;
    config->vsync = 1;
    config->pixel_format = PIXEL_FORMAT_RGB565;

    // Default RG35XX profile
    strcpy(config->profile.name, "Anbernic RG35XX");
    strcpy(config->profile.description, "RG35XX (H) - 640x480 LCD, 16-bit color");
    config->profile.screen_width = 640;
    config->profile.screen_height = 480;
    config->profile.scale = 2;
    config->profile.bpp = 16;
}

const char* platform_config_get_path(void) {
    static char path[512];

    // Try current directory first
    if (access("platform.conf", R_OK) == 0) {
        return "platform.conf";
    }

    // Try workspace/dev/
    if (access("workspace/dev/platform.conf", R_OK) == 0) {
        return "workspace/dev/platform.conf";
    }

    // Try ~/.minui/dev/
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.minui/dev/platform.conf", home);
        if (access(path, R_OK) == 0) {
            return path;
        }
    }

    return NULL;
}

int platform_config_load(const char* config_path, platform_config_t* config) {
    FILE* file;
    char line[512];
    char section[128] = "";
    char key[128], value[256];
    device_profile_t temp_profile;
    int have_temp_profile = 0;

    // Set defaults first
    platform_config_defaults(config);

    // If no path provided, try to find it
    if (!config_path) {
        config_path = platform_config_get_path();
        if (!config_path) {
            fprintf(stderr, "platform_config: No config file found, using defaults\n");
            return 0; // Not an error, just use defaults
        }
    }

    file = fopen(config_path, "r");
    if (!file) {
        fprintf(stderr, "platform_config: Cannot open %s, using defaults\n", config_path);
        return 0; // Not an error
    }

    printf("platform_config: Loading from %s\n", config_path);

    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim(line);

        // Skip empty lines and comments
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        // Check for section
        if (trimmed[0] == '[') {
            // Save previous profile if we were in one
            if (strncmp(section, "profile.", 8) == 0 && have_temp_profile) {
                const char* profile_name = section + 8;
                if (strcmp(profile_name, config->active_profile) == 0) {
                    memcpy(&config->profile, &temp_profile, sizeof(device_profile_t));
                }
            }

            parse_section(trimmed, section, sizeof(section));

            // Reset temp profile if entering a profile section
            if (strncmp(section, "profile.", 8) == 0) {
                memset(&temp_profile, 0, sizeof(device_profile_t));
                have_temp_profile = 1;
            }
            continue;
        }

        // Parse key=value
        if (!parse_keyvalue(trimmed, key, value, sizeof(value))) {
            continue;
        }

        // Handle [general] section
        if (strcmp(section, "general") == 0) {
            if (strcmp(key, "profile") == 0) {
                strncpy(config->active_profile, value, sizeof(config->active_profile) - 1);
            }
            else if (strcmp(key, "window_mode") == 0) {
                if (strcmp(value, "fullscreen") == 0) {
                    config->window_mode = WINDOW_MODE_FULLSCREEN;
                }
            }
            else if (strcmp(key, "vsync") == 0) {
                config->vsync = atoi(value);
            }
            else if (strcmp(key, "pixel_format") == 0) {
                if (strcmp(value, "ARGB8888") == 0) {
                    config->pixel_format = PIXEL_FORMAT_ARGB8888;
                }
            }
        }
        // Handle [profile.xxx] sections
        else if (strncmp(section, "profile.", 8) == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(temp_profile.name, value, sizeof(temp_profile.name) - 1);
            }
            else if (strcmp(key, "description") == 0) {
                strncpy(temp_profile.description, value, sizeof(temp_profile.description) - 1);
            }
            else if (strcmp(key, "screen_width") == 0) {
                temp_profile.screen_width = atoi(value);
            }
            else if (strcmp(key, "screen_height") == 0) {
                temp_profile.screen_height = atoi(value);
            }
            else if (strcmp(key, "scale") == 0) {
                temp_profile.scale = atoi(value);
            }
            else if (strcmp(key, "bpp") == 0) {
                temp_profile.bpp = atoi(value);
            }
        }
    }

    // Save last profile if we were in one
    if (strncmp(section, "profile.", 8) == 0 && have_temp_profile) {
        const char* profile_name = section + 8;
        if (strcmp(profile_name, config->active_profile) == 0) {
            memcpy(&config->profile, &temp_profile, sizeof(device_profile_t));
        }
    }

    fclose(file);

    printf("platform_config: Loaded profile '%s' - %s (%dx%d, %d-bit)\n",
        config->active_profile,
        config->profile.name,
        config->profile.screen_width,
        config->profile.screen_height,
        config->profile.bpp);

    return 0;
}
