#!/bin/bash
set -e

# Parse command-line arguments
# Preserve environment variables if already set
SCREEN_ARG=""
RECORD_ARG=""
RUN_MINUI_ARG=""
RUN_TESTS_ARG=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --screen)
      SCREEN_ARG="$2"
      shift 2
      ;;
    --record)
      RECORD_ARG="true"
      shift
      ;;
    --run-minui)
      RUN_MINUI_ARG="true"
      shift
      ;;
    --run-tests)
      RUN_TESTS_ARG="true"
      shift
      ;;
    *)
      echo "Unknown option: $1"
      shift
      ;;
  esac
done

# Apply CLI arguments if provided (overrides environment variables)
[ -n "$SCREEN_ARG" ] && SCREEN="$SCREEN_ARG"
[ -n "$RECORD_ARG" ] && RECORD="$RECORD_ARG"
[ -n "$RUN_MINUI_ARG" ] && RUN_MINUI="$RUN_MINUI_ARG"
[ -n "$RUN_TESTS_ARG" ] && RUN_TESTS="$RUN_TESTS_ARG"

# Determine SCREEN setting if not already set
# Priority order: CLI --screen flag > SCREEN env var > makefile.env > default
if [ -z "$SCREEN" ]; then
  # Try to parse from workspace/dev/platform/makefile.env
  for makefile_path in /work/src/workspace/dev/platform/makefile.env /work/src/workspace/platform/dev/makefile.env; do
    if [ -f "$makefile_path" ]; then
      # Look for SCREEN, SCREEN_SIZE, or SCREEN_GEOMETRY
      SCREEN_VAL=$(grep -E "^(SCREEN|SCREEN_SIZE|SCREEN_GEOMETRY)\s*=" "$makefile_path" | head -1 | cut -d= -f2 | tr -d ' ')
      if [ -n "$SCREEN_VAL" ]; then
        SCREEN="$SCREEN_VAL"
        break
      fi
    fi
  done
fi

# Default screen if not found
if [ -z "$SCREEN" ]; then
  SCREEN="640x480x24"
fi

# Parse SCREEN into WIDTH, HEIGHT, DEPTH
WIDTH=$(echo "$SCREEN" | cut -d'x' -f1)
HEIGHT=$(echo "$SCREEN" | cut -d'x' -f2)
DEPTH=$(echo "$SCREEN" | cut -d'x' -f3)

# Default depth if not specified
if [ -z "$DEPTH" ]; then
  DEPTH="24"
fi

echo "=================================================="
echo "MinUI Docker Development Environment"
echo "=================================================="
echo "Screen: ${WIDTH}x${HEIGHT}x${DEPTH}"
echo "Record: ${RECORD:-false}"
echo "Run MinUI: ${RUN_MINUI:-false}"
echo "Run Tests: ${RUN_TESTS:-false}"
echo "=================================================="

# Start Xvfb
echo "Starting Xvfb..."
Xvfb :99 -screen 0 ${WIDTH}x${HEIGHT}x${DEPTH} &
XVFB_PID=$!
export DISPLAY=:99

# Wait for X server to start
sleep 2

# Start x11vnc
echo "Starting x11vnc on port 5900..."
x11vnc -display :99 -nopw -forever -shared -rfbport 5900 &
X11VNC_PID=$!

# Wait for VNC to start
sleep 1

# Start recording if requested
if [ "$RECORD" = "true" ] || [ -n "$RECORD_VIDEO" ]; then
  TIMESTAMP=$(date +%Y%m%d_%H%M%S)
  mkdir -p /work/build/recordings
  RECORDING_FILE="/work/build/recordings/screen_${TIMESTAMP}.mp4"
  echo "Starting recording to: $RECORDING_FILE"
  ffmpeg -f x11grab -video_size ${WIDTH}x${HEIGHT} -i :99 -codec:v libx264 -r 30 "$RECORDING_FILE" &
  FFMPEG_PID=$!
fi

# Determine what to run
if [ "$RUN_MINUI" = "true" ]; then
  # Run MinUI binary
  MINUI_BIN="${MINUI_BIN:-/work/build/SYSTEM/dev/bin/minui.elf}"
  
  if [ ! -f "$MINUI_BIN" ]; then
    echo "ERROR: MinUI binary not found at: $MINUI_BIN"
    echo "Please build MinUI first or set MINUI_BIN environment variable"
    exit 1
  fi
  
  echo "Running MinUI: $MINUI_BIN"
  "$MINUI_BIN" &
  MINUI_PID=$!
  
  # Wait for MinUI process
  wait $MINUI_PID
  EXIT_CODE=$?
  echo "MinUI exited with code: $EXIT_CODE"
  
elif [ "$RUN_TESTS" = "true" ] || [ -z "$RUN_MINUI" ]; then
  # Default: Run test runner
  TEST_CONFIG="${TEST_CONFIG:-/work/src/workspace/dev/tests/test_basic_navigation.json}"
  TEST_RUNNER="/work/src/workspace/dev/tools/run_tests.py"
  
  if [ ! -f "$TEST_RUNNER" ]; then
    echo "ERROR: Test runner not found at: $TEST_RUNNER"
    exit 1
  fi
  
  echo "Running test runner..."
  echo "Test config: $TEST_CONFIG"
  
  cd /work/src/workspace/dev/tools
  /home/dev/venv/bin/python3 run_tests.py --tests /work/src/workspace/dev/tests --output /work/build/test_output --headless || true
  EXIT_CODE=$?
  echo "Test runner exited with code: $EXIT_CODE"
fi

echo "=================================================="
echo "Container is staying alive for inspection"
echo "Connect to VNC at localhost:5900 to view display"
echo "Press Ctrl+C to exit"
echo "=================================================="

# Keep container alive for inspection
tail -f /dev/null
