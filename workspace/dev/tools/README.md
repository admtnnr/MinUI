# MinUI Dev Platform Testing Tools

This directory contains automation tools for testing the MinUI dev platform (Phase 3.4).

## Tools Overview

### 1. run_tests.py
**Test Suite Runner** - Automated testing framework

Runs test suites, manages MinUI process, and generates reports.

```bash
# Run all tests
./run_tests.py

# Run in headless mode (for CI)
./run_tests.py --headless

# Custom configuration
./run_tests.py \
  --minui ../all/minui/build/dev/minui.elf \
  --tests ../tests \
  --output ../test_output
```

**Features:**
- Automated test execution
- Screenshot capture and comparison
- HTML and JSON report generation
- Headless mode for CI/CD
- Input playback support

### 2. input_player.py
**Input Playback Tool** - Replays recorded input sequences

Simulates keyboard input to replay recorded user interactions.

```bash
# Play a recording
./input_player.py recording.csv

# Play at 2x speed
./input_player.py recording.csv --speed 2.0

# Add initial delay
./input_player.py recording.csv --delay 3.0

# Target specific window
./input_player.py recording.csv --window "MinUI Dev - RG35XX"
```

**Requirements:**
- `python3-xlib` - Install with: `pip3 install python-xlib`

**Input Recording Format:**
CSV file with timestamp and button state:
```csv
# MinUI Input Recording
# Format: timestamp,button_state
1000,16
1100,0
1200,32
```

Button state is a bitmask of pressed buttons (see platform.h for values).

### 3. compare_images.py
**Visual Comparison Tool** - Compares screenshots for regression testing

Compares two images pixel-by-pixel and generates difference visualization.

```bash
# Compare two images
./compare_images.py expected.png actual.png

# Custom threshold (default: 0.95)
./compare_images.py expected.png actual.png --threshold 0.98

# Save diff visualization
./compare_images.py expected.png actual.png --output diff.png

# Quiet mode (only show pass/fail)
./compare_images.py expected.png actual.png -q
```

**Requirements:**
- `Pillow` - Install with: `pip3 install Pillow`

**Output:**
- Exit code 0 = images match (above threshold)
- Exit code 1 = images differ (below threshold)
- Optional diff image showing side-by-side comparison

## Installation

### Ubuntu/Debian
```bash
sudo apt-get install python3 python3-pip python3-xlib
pip3 install Pillow python-xlib
```

### macOS
```bash
brew install python3
pip3 install Pillow pyobjc-framework-Quartz pyobjc-framework-ApplicationServices
```

Note: macOS support for input_player.py requires additional work (Quartz events).

## Workflow

### 1. Record Input Sequence
```bash
# Build and run MinUI
export CROSS_COMPILE=" " && make PLATFORM=dev
cd ../all/minui/build/dev
./minui.elf

# During runtime:
# - Press F11 to start recording
# - Perform actions
# - Press F11 to stop recording
# Output: input_recording_YYYYMMDD_HHMMSS.csv
```

### 2. Capture Reference Screenshots
```bash
# Run MinUI and navigate to desired state
./minui.elf

# Press F12 to capture screenshot
# Screenshots saved to: ./screenshots/screenshot_YYYYMMDD_HHMMSS.png

# Move to test expectations
mv screenshots/screenshot_*.png ../tests/expected/
```

### 3. Create Test Case
```json
{
  "name": "My Test",
  "steps": [
    {"action": "wait", "duration": 2000},
    {"action": "play_input", "recording": "my_recording.csv"},
    {"action": "screenshot", "name": "result"},
    {"action": "compare", "expected": "expected/result.png", "threshold": 0.95}
  ]
}
```

### 4. Run Tests
```bash
cd tools
./run_tests.py
```

### 5. Review Results
- Open `test_output/test_report.html` in browser
- Check diff images for failed comparisons
- Review JSON report for automation

## Environment Variables

### MINUI_HEADLESS
Run MinUI in headless mode (no visible window).
```bash
MINUI_HEADLESS=1 ./minui.elf
```

### MINUI_SCREENSHOTS_DIR
Custom directory for screenshots.
```bash
MINUI_SCREENSHOTS_DIR=/tmp/screenshots ./minui.elf
```

## Keyboard Shortcuts (During MinUI Runtime)

- **F12** - Capture screenshot
- **F11** - Start/stop input recording

## CI/CD Integration

### GitHub Actions
See `.github/workflows/test-dev-platform.yml` for automated testing configuration.

```yaml
- name: Run tests
  run: |
    Xvfb :99 -screen 0 640x480x24 &
    export DISPLAY=:99
    cd workspace/dev/tools
    python3 run_tests.py --headless
```

### GitLab CI
```yaml
test:
  stage: test
  script:
    - apt-get install -y xvfb python3-xlib python3-pil
    - Xvfb :99 &
    - export DISPLAY=:99
    - cd workspace/dev/tools
    - python3 run_tests.py --headless
  artifacts:
    paths:
      - workspace/test_output/
    when: always
```

## Troubleshooting

### "Window not found" error (input_player.py)
- Ensure MinUI is running before starting playback
- Check window title matches (use `--window` argument)
- On Linux, ensure X11 is running (not Wayland)

### Screenshot capture not working
- Ensure SDL2_image is installed
- Check screenshots directory permissions
- Verify F12 key is not captured by window manager

### Headless mode issues
- Ensure Xvfb is running: `Xvfb :99 -screen 0 640x480x24 &`
- Set DISPLAY: `export DISPLAY=:99`
- Check Xvfb process: `ps aux | grep Xvfb`

### Image comparison always fails
- Check image formats match (PNG recommended)
- Verify image sizes match
- Adjust threshold (try 0.90 for more tolerance)
- Review diff image to see actual differences

## Example Test Suite

See `../tests/` for example test cases and expected screenshots.

## Future Enhancements

Phase 3.4 could be extended with:
- [ ] Automated input generation (fuzzing)
- [ ] Performance benchmarking (FPS, frame time)
- [ ] Memory leak detection
- [ ] Code coverage integration
- [ ] macOS/Windows input playback support
- [ ] Video recording of test runs
- [ ] Parallel test execution

## Support

For issues or questions:
1. Check test logs in `test_output/`
2. Review platform.c logs
3. Verify dependencies are installed
4. See main README.md for platform documentation
