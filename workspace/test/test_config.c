#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

// Stub logging functions required by config.c
void LOG_info(const char* fmt, ...) { (void)fmt; }
void LOG_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "WARN: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}
void LOG_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}
void LOG_note(int level, const char* fmt, ...) { (void)level; (void)fmt; }

#define USE_CONFIG_SYSTEM 1
#include "../all/common/config.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("\nTest: %s\n", name); \
    printf("-----------------------------------\n")

#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAILED: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

void test_parsing() {
    TEST("Config Parsing");

    // Create test config file
    FILE* f = fopen("/tmp/minui_test.conf", "w");
    fprintf(f, "# Test configuration\n");
    fprintf(f, "display_sharpness=sharp\n");
    fprintf(f, "display_scale=fullscreen\n");
    fprintf(f, "display_vsync=0\n");
    fprintf(f, "show_fps=1\n");
    fprintf(f, "fast_forward_speed=8\n");
    fprintf(f, "thread_video=1\n");
    fprintf(f, "audio_latency=128\n");
    fprintf(f, "cpu_speed_menu=600\n");
    fprintf(f, "debug=1\n");
    fclose(f);

    // Load and verify
    minui_config_t* config = config_load("/tmp/minui_test.conf");
    ASSERT(config != NULL, "Config loaded successfully");

    if (config) {
        ASSERT(config->display_sharpness == DISPLAY_SHARP, "display_sharpness = sharp");
        ASSERT(config->display_scale == DISPLAY_SCALE_FULLSCREEN, "display_scale = fullscreen");
        ASSERT(config->display_vsync == 0, "display_vsync = 0");
        ASSERT(config->show_fps == 1, "show_fps = 1");
        ASSERT(config->fast_forward_speed == 8, "fast_forward_speed = 8");
        ASSERT(config->thread_video == 1, "thread_video = 1");
        ASSERT(config->audio_latency == 128, "audio_latency = 128");
        ASSERT(config->cpu_speed_menu == 600, "cpu_speed_menu = 600");
        ASSERT(config->debug == 1, "debug = 1");

        config_free(config);
    }

    unlink("/tmp/minui_test.conf");
}

void test_all_sharpness_values() {
    TEST("All Sharpness Values");

    const char* sharpness_values[] = {"sharp", "crisp", "soft"};
    display_sharpness_t expected[] = {DISPLAY_SHARP, DISPLAY_CRISP, DISPLAY_SOFT};

    for (int i = 0; i < 3; i++) {
        FILE* f = fopen("/tmp/minui_test.conf", "w");
        fprintf(f, "display_sharpness=%s\n", sharpness_values[i]);
        fclose(f);

        minui_config_t* config = config_load("/tmp/minui_test.conf");
        char msg[64];
        snprintf(msg, sizeof(msg), "display_sharpness=%s", sharpness_values[i]);
        ASSERT(config && config->display_sharpness == expected[i], msg);
        config_free(config);
    }

    unlink("/tmp/minui_test.conf");
}

void test_all_scale_values() {
    TEST("All Scale Values");

    const char* scale_values[] = {"aspect", "fullscreen", "integer", "native"};
    display_scale_t expected[] = {DISPLAY_SCALE_ASPECT, DISPLAY_SCALE_FULLSCREEN,
                                   DISPLAY_SCALE_INTEGER, DISPLAY_SCALE_NATIVE};

    for (int i = 0; i < 4; i++) {
        FILE* f = fopen("/tmp/minui_test.conf", "w");
        fprintf(f, "display_scale=%s\n", scale_values[i]);
        fclose(f);

        minui_config_t* config = config_load("/tmp/minui_test.conf");
        char msg[64];
        snprintf(msg, sizeof(msg), "display_scale=%s", scale_values[i]);
        ASSERT(config && config->display_scale == expected[i], msg);
        config_free(config);
    }

    unlink("/tmp/minui_test.conf");
}

void test_defaults() {
    TEST("Default Values");

    minui_config_t* config = config_load("/nonexistent/path.conf");
    ASSERT(config != NULL, "Config created with defaults");

    if (config) {
        ASSERT(config->display_sharpness == DISPLAY_SOFT, "Default sharpness = soft");
        ASSERT(config->display_scale == DISPLAY_SCALE_ASPECT, "Default scale = aspect");
        ASSERT(config->display_vsync == 1, "Default vsync = 1");
        ASSERT(config->show_fps == 0, "Default show_fps = 0");
        ASSERT(config->fast_forward_speed == 3, "Default fast_forward_speed = 3");
        ASSERT(config->thread_video == 0, "Default thread_video = 0");
        ASSERT(config->audio_latency == 64, "Default audio_latency = 64");
        ASSERT(config->debug == 0, "Default debug = 0");
        ASSERT(strcmp(config->graphics_backend, "auto") == 0, "Default graphics_backend = auto");

        config_free(config);
    }
}

void test_comments_and_empty_lines() {
    TEST("Comments and Empty Lines");

    FILE* f = fopen("/tmp/minui_test.conf", "w");
    fprintf(f, "# This is a comment\n");
    fprintf(f, "\n");
    fprintf(f, "show_fps=1\n");
    fprintf(f, "  # Indented comment\n");
    fprintf(f, "\n");
    fprintf(f, "debug=1\n");
    fprintf(f, "\n");
    fclose(f);

    minui_config_t* config = config_load("/tmp/minui_test.conf");
    ASSERT(config != NULL, "Loaded config with comments");
    ASSERT(config && config->show_fps == 1, "show_fps = 1 (after comment)");
    ASSERT(config && config->debug == 1, "debug = 1 (after empty lines)");

    config_free(config);
    unlink("/tmp/minui_test.conf");
}

void test_paths() {
    TEST("Path Settings");

    FILE* f = fopen("/tmp/minui_test.conf", "w");
    fprintf(f, "rom_path=/custom/roms\n");
    fprintf(f, "bios_path=/custom/bios\n");
    fprintf(f, "saves_path=/custom/saves\n");
    fclose(f);

    minui_config_t* config = config_load("/tmp/minui_test.conf");
    ASSERT(config != NULL, "Loaded config with paths");
    ASSERT(config && strcmp(config->rom_path, "/custom/roms") == 0, "rom_path correct");
    ASSERT(config && strcmp(config->bios_path, "/custom/bios") == 0, "bios_path correct");
    ASSERT(config && strcmp(config->saves_path, "/custom/saves") == 0, "saves_path correct");

    config_free(config);
    unlink("/tmp/minui_test.conf");
}

void test_cpu_speeds() {
    TEST("CPU Speed Settings");

    FILE* f = fopen("/tmp/minui_test.conf", "w");
    fprintf(f, "cpu_speed_menu=600\n");
    fprintf(f, "cpu_speed_powersave=800\n");
    fprintf(f, "cpu_speed_normal=1200\n");
    fprintf(f, "cpu_speed_performance=1500\n");
    fclose(f);

    minui_config_t* config = config_load("/tmp/minui_test.conf");
    ASSERT(config != NULL, "Loaded config with CPU speeds");
    ASSERT(config && config->cpu_speed_menu == 600, "cpu_speed_menu = 600");
    ASSERT(config && config->cpu_speed_powersave == 800, "cpu_speed_powersave = 800");
    ASSERT(config && config->cpu_speed_normal == 1200, "cpu_speed_normal = 1200");
    ASSERT(config && config->cpu_speed_performance == 1500, "cpu_speed_performance = 1500");

    config_free(config);
    unlink("/tmp/minui_test.conf");
}

int main() {
    printf("===========================================\n");
    printf("MinUI Configuration System Tests\n");
    printf("===========================================\n");

    test_parsing();
    test_all_sharpness_values();
    test_all_scale_values();
    test_defaults();
    test_comments_and_empty_lines();
    test_paths();
    test_cpu_speeds();

    printf("\n===========================================\n");
    printf("Test Results\n");
    printf("===========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("\n");

    if (tests_failed == 0) {
        printf("✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("✗ Some tests failed\n\n");
        return 1;
    }
}
