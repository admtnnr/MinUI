# MinUI Test Suite

This directory contains automated test cases for the MinUI dev platform.

## Test File Format

Tests are defined in JSON format with the following structure:

```json
{
  "name": "Test Name",
  "description": "Test description",
  "steps": [
    {
      "action": "wait",
      "duration": 1000,
      "comment": "Wait 1 second"
    },
    {
      "action": "screenshot",
      "name": "screenshot_name"
    },
    {
      "action": "play_input",
      "recording": "input_recording.csv",
      "speed": 1.0
    },
    {
      "action": "compare",
      "expected": "expected/screenshot.png",
      "actual": "latest",
      "threshold": 0.95
    }
  ]
}
```

## Available Actions

### wait
Wait for a specified duration.
- `duration`: Time in milliseconds

### screenshot
Capture a screenshot.
- `name`: Name for the screenshot (optional)

### play_input
Play a recorded input sequence.
- `recording`: Path to input recording file (relative to tests directory)
- `speed`: Playback speed multiplier (optional, default: 1.0)

### compare
Compare screenshots for visual regression testing.
- `expected`: Path to expected image (relative to tests directory)
- `actual`: Path to actual image, or "latest" for most recent screenshot
- `threshold`: Similarity threshold (0-1, default: 0.95)

## Directory Structure

```
tests/
├── README.md                  # This file
├── test_*.json                # Test case definitions
├── expected/                  # Expected/reference screenshots
│   └── *.png
└── recordings/                # Input recordings
    └── *.csv
```

## Running Tests

### Run all tests:
```bash
cd workspace/dev/tools
python3 run_tests.py
```

### Run in headless mode (for CI):
```bash
python3 run_tests.py --headless
```

### Custom paths:
```bash
python3 run_tests.py \
  --minui ../all/minui/build/dev/minui.elf \
  --tests ../tests \
  --output ../test_output
```

## Creating New Tests

1. **Record input sequence** (optional):
   - Run MinUI normally
   - Press F11 to start recording
   - Perform actions
   - Press F11 again to stop
   - Input recording saved to `input_recording_YYYYMMDD_HHMMSS.csv`

2. **Capture reference screenshots**:
   - Run MinUI normally
   - Navigate to desired state
   - Press F12 to capture screenshot
   - Move screenshot to `tests/expected/`

3. **Create test JSON file**:
   - Create `tests/test_yourtest.json`
   - Define test steps
   - Add screenshot comparisons

4. **Run test**:
   ```bash
   python3 tools/run_tests.py
   ```

## Example Test

See `test_basic_navigation.json` for a simple example.

## Test Output

Test results are saved to `test_output/` (or custom directory):
- `test_report.json` - Machine-readable test results
- `test_report.html` - Human-readable HTML report
- `screenshots/` - Captured screenshots during test run
- `diff_*.png` - Visual diffs for failed comparisons

## Continuous Integration

Tests can run automatically in CI/CD pipelines using headless mode:

```yaml
# GitHub Actions example
- name: Run MinUI tests
  run: |
    cd workspace/dev/tools
    python3 run_tests.py --headless
```

See `.github/workflows/test-dev-platform.yml` for full CI configuration.
