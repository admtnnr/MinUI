# Automated Testing Platform for MinUI

## Executive Summary

**Yes**, a generic test platform for QEMU or native Linux would be sufficient for automated testing of MinUI configuration options.

The macOS platform demonstrates that MinUI can run on non-embedded systems with minimal platform implementation. We can create a similar "test" or "linux-native" platform specifically for CI/automated testing.

---

## Requirements for Automated Testing

### 1. Build Requirements

**What we need:**
- Native GCC compiler (x86_64 Linux)
- SDL2 development libraries (`libsdl2-dev`)
- Standard Linux development tools (make, etc.)
- No cross-compiler or device-specific toolchains

**What we have:**
- Already have GCC available in this environment
- SDL2 can be installed via package manager
- MinUI's build system already supports multiple platforms

### 2. Test Platform Implementation

**Minimal platform implementation:**

```
workspace/test/
├── platform/
│   ├── platform.h       # Define constants (screen size, buttons, paths)
│   ├── platform.c       # Implement PLAT_* functions
│   ├── makefile.env     # Build configuration (use native gcc, SDL2)
│   └── phase2_flags.mk  # Enable USE_CONFIG_SYSTEM=1
└── test_config.conf     # Test configuration file
```

**Required PLAT_* functions** (based on macos platform):
- `PLAT_initVideo()` - Create SDL2 window/renderer
- `PLAT_quitVideo()` - Cleanup SDL2
- `PLAT_clearVideo()` - Clear screen
- `PLAT_flip()` - Present frame
- `PLAT_setVsync()` - Control vsync
- `PLAT_setSharpness()` - Set rendering sharpness
- `PLAT_setCPUSpeed()` - Stub (log only)
- `PLAT_setRumble()` - Stub (no-op)
- `PLAT_getBatteryStatus()` - Stub (return fake values)
- `PLAT_initInput()` - Initialize SDL2 input
- `PLAT_quitInput()` - Cleanup input
- Power management stubs (InitSettings, QuitSettings, etc.)

**Most functions can be stubs** - we're testing configuration loading and application, not actual hardware behavior.

### 3. Test Harness

**Approach 1: Unit Tests** (test config parsing only)
```c
// test_config_parse.c
int main() {
    minui_config_t* config = config_load("test_config.conf");
    assert(config != NULL);
    assert(config->display_sharpness == DISPLAY_SHARP);
    assert(config->display_scale == DISPLAY_SCALE_FULLSCREEN);
    // ... test all options
    config_free(config);
    return 0;
}
```

**Approach 2: Integration Tests** (test config application)
```c
// test_config_apply.c - Run minarch with test config
int main(int argc, char** argv) {
    // Load config
    minui_config_t* config = config_load("test_config.conf");
    CONFIG_set(config);

    // Initialize systems (would normally be in minarch main)
    // ... init video, audio, etc ...

    // VERIFY: Check that config was applied
    extern int screen_sharpness;  // From minarch.c
    assert(screen_sharpness == SHARPNESS_SHARP);

    extern int screen_scaling;
    assert(screen_scaling == SCALE_FULLSCREEN);

    // ... verify all applied settings

    printf("All tests passed!\n");
    return 0;
}
```

**Approach 3: Functional Tests** (run actual emulator)
```c
// Run minarch with test ROM for N frames, verify config applied
// Could use a simple test ROM that exits immediately
// Check log output for expected values
```

---

## What Can Be Tested

### ✅ Fully Testable

These can be tested programmatically:

1. **Config parsing** - Verify file is loaded and parsed correctly
2. **Config validation** - Test invalid values are rejected/clamped
3. **Config application** - Verify variables are set correctly
4. **Global config access** - Test CONFIG_get/CONFIG_set functions
5. **Default fallback** - Test behavior when config file missing
6. **Enum mappings** - Verify DISPLAY_* → SCALE_* conversions

### ⚠️ Partially Testable

These can be partially verified:

7. **display_sharpness** - Can check variable is set, hard to verify visual output
8. **display_vsync** - Can check vsync state, hard to verify actual sync
9. **show_fps** - Can check flag is set, would need screenshot to verify display
10. **fast_forward_speed** - Can check variable, could test timing with mock frames

### ❌ Hard to Test Automatically

These require visual inspection or device hardware:

11. **CPU speed** - Requires real hardware with frequency control
12. **Audio latency** - Requires ear or audio analysis tools
13. **Rumble** - Requires physical vibration motor
14. **Battery** - Requires real battery

---

## Implementation Phases

### Phase 1: Basic Config Test (1-2 hours)

**Goal**: Verify config parsing works

```bash
workspace/test/test_config_parse
├── test_config_parse.c  # Simple unit test
├── test_config.conf      # Test configuration
└── Makefile              # Build with gcc + SDL2
```

**Test**:
```bash
cd workspace/test
make test_config_parse
./test_config_parse
```

### Phase 2: Platform Implementation (3-4 hours)

**Goal**: Create minimal test platform

```bash
workspace/test/platform/
├── platform.h            # Constants
├── platform.c            # Stub implementations
├── makefile.env          # ARCH=-O0 -g, SDL=SDL2
└── phase2_flags.mk       # USE_CONFIG_SYSTEM=1
```

**Test**:
```bash
cd workspace
make PLATFORM=test
```

### Phase 3: Integration Tests (2-3 hours)

**Goal**: Verify config is applied in minarch/minui

```bash
workspace/test/
├── test_minarch_config.c  # Test minarch config application
├── test_minui_config.c    # Test minui config application
└── run_tests.sh           # Run all tests
```

**Test**:
```bash
cd workspace/test
./run_tests.sh
# Expected output:
# ✓ Config parsing tests passed
# ✓ Config application tests passed
# ✓ Integration tests passed
```

### Phase 4: CI/CD Integration (1-2 hours)

**Goal**: Run tests automatically on every commit

```.github/workflows/test.yml
name: MinUI Config Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install -y libsdl2-dev
      - name: Build test platform
        run: cd workspace && make PLATFORM=test
      - name: Run tests
        run: cd workspace/test && ./run_tests.sh
```

---

## Recommended Approach

**Start with Phase 1** - It's the easiest and gives immediate value:

1. Create `workspace/test/test_config_parse.c`
2. Test that config_load() works correctly
3. Test all 23 config options are parsed
4. Verify enum conversions
5. Test validation and clamping
6. Test missing file behavior

**This requires:**
- ~50 lines of C code
- Simple Makefile
- No platform implementation needed
- Can run in 5 minutes

**Then decide** if Phases 2-4 are worth the effort based on:
- How much manual testing do you want to avoid?
- Will you have multiple contributors?
- How often do config options change?

---

## Example: Minimal Test Implementation

```c
// workspace/test/test_config.c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// We need to provide these stubs for linking
void LOG_info(const char* fmt, ...) { }
void LOG_warn(const char* fmt, ...) { }
void LOG_error(const char* fmt, ...) { }

#define USE_CONFIG_SYSTEM 1
#include "../all/common/config.h"

void test_parsing() {
    printf("Testing config parsing...\n");

    // Create test config file
    FILE* f = fopen("/tmp/test.conf", "w");
    fprintf(f, "display_sharpness=sharp\n");
    fprintf(f, "display_scale=fullscreen\n");
    fprintf(f, "display_vsync=0\n");
    fprintf(f, "show_fps=1\n");
    fprintf(f, "fast_forward_speed=8\n");
    fclose(f);

    // Load and verify
    minui_config_t* config = config_load("/tmp/test.conf");
    assert(config != NULL);
    assert(config->display_sharpness == DISPLAY_SHARP);
    assert(config->display_scale == DISPLAY_SCALE_FULLSCREEN);
    assert(config->display_vsync == 0);
    assert(config->show_fps == 1);
    assert(config->fast_forward_speed == 8);

    config_free(config);
    unlink("/tmp/test.conf");

    printf("✓ Parsing tests passed\n");
}

void test_defaults() {
    printf("Testing default values...\n");

    minui_config_t* config = config_load("/nonexistent/path.conf");
    assert(config != NULL);  // Should return defaults, not NULL
    assert(config->display_sharpness == DISPLAY_SOFT);  // Default
    assert(config->display_scale == DISPLAY_SCALE_ASPECT);  // Default

    config_free(config);

    printf("✓ Default tests passed\n");
}

void test_validation() {
    printf("Testing validation...\n");

    FILE* f = fopen("/tmp/test.conf", "w");
    fprintf(f, "fast_forward_speed=999\n");  // Invalid (max is 10)
    fprintf(f, "audio_latency=5\n");  // Invalid (min is 32)
    fclose(f);

    minui_config_t* config = config_load("/tmp/test.conf");
    // Values should be clamped
    assert(config->fast_forward_speed <= 10);
    assert(config->audio_latency >= 32);

    config_free(config);
    unlink("/tmp/test.conf");

    printf("✓ Validation tests passed\n");
}

int main() {
    test_parsing();
    test_defaults();
    test_validation();

    printf("\nAll tests passed! ✓\n");
    return 0;
}
```

**Compile:**
```bash
gcc -o test_config test_config.c ../all/common/config.c -I../all/common
```

**Run:**
```bash
./test_config
```

---

## Limitations

### What automated tests CANNOT verify:

1. **Visual correctness** - We can check `screen_sharpness = SHARPNESS_SHARP` but not if rendering looks sharp
2. **Performance** - Can't measure actual FPS or latency without real hardware
3. **Hardware behavior** - Can't test CPU frequency changes, battery status, etc.
4. **User experience** - Can't test if menus feel responsive or if vsync eliminates tearing

### What still requires manual testing:

- Visual quality of different sharpness/scaling modes
- Audio latency perception
- Performance on target devices
- Battery life impact
- Overall user experience

---

## Conclusion

**Yes, automated testing is feasible and valuable**, especially for:
- Regression testing (ensure config changes don't break parsing)
- CI/CD (catch errors before they reach users)
- Faster development iteration (test without deploying to device)

**Recommended next steps:**
1. Implement Phase 1 unit tests (minimal effort, high value)
2. Evaluate if Phase 2-4 are worth the investment
3. Continue manual testing on real hardware for visual/UX validation

**Time investment:** ~2-10 hours depending on scope
**Value:** Catch bugs early, enable confident refactoring, faster iteration

Would you like me to implement Phase 1 (basic config parsing tests)?
