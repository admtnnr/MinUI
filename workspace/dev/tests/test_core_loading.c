/*
 * Unit test for minarch core loading
 * Verifies that libretro cores can be loaded and contain required symbols
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#define TEST_PASS "\033[32mPASS\033[0m"
#define TEST_FAIL "\033[31mFAIL\033[0m"

int tests_passed = 0;
int tests_failed = 0;

void test_core_loading(const char* core_path) {
    printf("Testing core loading: %s\n", core_path);

    // Test 1: Core file exists and can be opened
    void* handle = dlopen(core_path, RTLD_LAZY);
    if (!handle) {
        printf("  [%s] dlopen core: %s\n", TEST_FAIL, dlerror());
        tests_failed++;
        return;
    }
    printf("  [%s] dlopen core\n", TEST_PASS);
    tests_passed++;

    // Test 2: Required libretro API functions exist
    const char* required_symbols[] = {
        "retro_init",
        "retro_deinit",
        "retro_get_system_info",
        "retro_get_system_av_info",
        "retro_set_environment",
        "retro_set_video_refresh",
        "retro_set_audio_sample",
        "retro_set_input_poll",
        "retro_set_input_state",
        "retro_load_game",
        "retro_unload_game",
        "retro_run",
        "retro_reset",
        NULL
    };

    for (int i = 0; required_symbols[i] != NULL; i++) {
        void* sym = dlsym(handle, required_symbols[i]);
        if (!sym) {
            printf("  [%s] Symbol '%s': %s\n", TEST_FAIL, required_symbols[i], dlerror());
            tests_failed++;
        } else {
            printf("  [%s] Symbol '%s' found\n", TEST_PASS, required_symbols[i]);
            tests_passed++;
        }
    }

    // Test 3: Get system info
    typedef struct {
        const char* library_name;
        const char* library_version;
        const char* valid_extensions;
        int need_fullpath;
        int block_extract;
    } retro_system_info;

    typedef void (*get_system_info_t)(retro_system_info*);
    get_system_info_t get_system_info = (get_system_info_t)dlsym(handle, "retro_get_system_info");

    if (get_system_info) {
        retro_system_info info = {0};
        get_system_info(&info);

        if (info.library_name && strlen(info.library_name) > 0) {
            printf("  [%s] Core library_name: %s\n", TEST_PASS, info.library_name);
            tests_passed++;
        } else {
            printf("  [%s] Core library_name is empty\n", TEST_FAIL);
            tests_failed++;
        }

        if (info.library_version && strlen(info.library_version) > 0) {
            printf("  [%s] Core library_version: %s\n", TEST_PASS, info.library_version);
            tests_passed++;
        } else {
            printf("  [%s] Core library_version is empty\n", TEST_FAIL);
            tests_failed++;
        }

        if (info.valid_extensions && strlen(info.valid_extensions) > 0) {
            printf("  [%s] Core valid_extensions: %s\n", TEST_PASS, info.valid_extensions);
            tests_passed++;
        } else {
            printf("  [%s] Core valid_extensions is empty\n", TEST_FAIL);
            tests_failed++;
        }
    }

    dlclose(handle);
}

int main(int argc, char* argv[]) {
    printf("=== MinArch Core Loading Tests ===\n\n");

    const char* core_path = argc > 1 ? argv[1] : "../cores/output/gambatte_libretro.so";
    test_core_loading(core_path);

    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
