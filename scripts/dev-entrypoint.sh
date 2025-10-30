#!/bin/bash
# MinUI Development Environment Entrypoint
# Manages Xvfb, VNC, and test runner or MinUI execution

set -e

# Default values (24-bit is standard for Xvfb, even though platform uses 32-bit RGBA)
SCREEN_SIZE="${SCREEN:-640x480x24}"
RUN_MINUI="${RUN_MINUI:-false}"
RECORD="${RECORD:-false}"
TEST_DIR="${TEST_DIR:-workspace/dev/tests}"
TEST_OUTPUT="${TEST_OUTPUT:-workspace/dev/test_output}"
MINUI_BIN="${MINUI_BIN:-./build/SYSTEM/rg35xx/bin/minui.elf}"
DISPLAY_NUM="${DISPLAY_NUM:-99}"
VNC_PORT="${VNC_PORT:-5900}"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --screen)
            SCREEN_SIZE="$2"
            shift 2
            ;;
        --minui)
            RUN_MINUI="true"
            shift
            ;;
        --record)
            RECORD="true"
            shift
            ;;
        --tests)
            TEST_DIR="$2"
            shift 2
            ;;
        --output)
            TEST_OUTPUT="$2"
            shift 2
            ;;
        --help)
            echo "MinUI Development Environment"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --screen SIZE      Screen resolution (default: 640x480x24 or from platform files)"
            echo "  --minui            Run MinUI binary instead of test runner"
            echo "  --record           Record screen to video file"
            echo "  --tests DIR        Test directory (default: workspace/dev/tests)"
            echo "  --output DIR       Test output directory (default: workspace/dev/test_output)"
            echo "  --help             Show this help message"
            echo ""
            echo "Environment Variables:"
            echo "  SCREEN             Override screen resolution"
            echo "  RUN_MINUI          Set to 'true' to run MinUI binary"
            echo "  RECORD             Set to 'true' to enable recording"
            echo "  TEST_DIR           Test directory path"
            echo "  TEST_OUTPUT        Test output directory path"
            echo "  MINUI_BIN          Path to MinUI binary"
            echo "  DISPLAY_NUM        X display number (default: 99)"
            echo "  VNC_PORT           VNC server port (default: 5900)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to detect screen size from platform files
detect_screen_size() {
    local detected=""
    
    # Try to find screen size in workspace/dev/platform files
    if [ -f "workspace/dev/platform/platform.h" ]; then
        local width=$(grep -E "^#define\s+FIXED_WIDTH" workspace/dev/platform/platform.h | awk '{print $3}')
        local height=$(grep -E "^#define\s+FIXED_HEIGHT" workspace/dev/platform/platform.h | awk '{print $3}')
        local bpp=$(grep -E "^#define\s+FIXED_BPP" workspace/dev/platform/platform.h | awk '{print $3}')
        
        if [ -n "$width" ] && [ -n "$height" ] && [ -n "$bpp" ]; then
            # Use 24-bit depth for Xvfb (standard), even though platform uses 32-bit RGBA
            detected="${width}x${height}x24"
        fi
    fi
    
    echo "$detected"
}

# Determine final screen size (priority: CLI arg > env var > detected > default)
if [ -z "$SCREEN" ]; then
    # Try to detect from platform files if not overridden
    DETECTED_SIZE=$(detect_screen_size)
    if [ -n "$DETECTED_SIZE" ]; then
        SCREEN_SIZE="$DETECTED_SIZE"
    fi
fi

echo "================================"
echo "MinUI Development Environment"
echo "================================"
echo "Screen size: $SCREEN_SIZE"
echo "Display: :$DISPLAY_NUM"
echo "VNC port: $VNC_PORT"
echo "Record: $RECORD"
echo "Mode: $([ "$RUN_MINUI" = "true" ] && echo "MinUI" || echo "Test Runner")"
echo "================================"

# Start Xvfb
echo "Starting Xvfb..."
Xvfb :$DISPLAY_NUM -screen 0 $SCREEN_SIZE -ac +extension GLX +render -noreset &
XVFB_PID=$!

# Wait for Xvfb to start
sleep 2

# Check if Xvfb started successfully
if ! kill -0 $XVFB_PID 2>/dev/null; then
    echo "Error: Xvfb failed to start"
    exit 1
fi

echo "Xvfb started (PID: $XVFB_PID)"

# Set DISPLAY for subsequent commands
export DISPLAY=:$DISPLAY_NUM

# Start x11vnc
echo "Starting x11vnc..."
x11vnc -display :$DISPLAY_NUM -nopw -forever -shared -rfbport $VNC_PORT -bg -o /tmp/x11vnc.log

# Wait for x11vnc to start
sleep 1

echo "x11vnc started on port $VNC_PORT"

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    
    # Stop recording if active
    if [ -n "$FFMPEG_PID" ] && kill -0 $FFMPEG_PID 2>/dev/null; then
        echo "Stopping recording..."
        kill -INT $FFMPEG_PID 2>/dev/null || true
        wait $FFMPEG_PID 2>/dev/null || true
    fi
    
    # Stop x11vnc
    pkill -f "x11vnc.*:$DISPLAY_NUM" 2>/dev/null || true
    
    # Stop Xvfb
    if [ -n "$XVFB_PID" ] && kill -0 $XVFB_PID 2>/dev/null; then
        echo "Stopping Xvfb..."
        kill $XVFB_PID 2>/dev/null || true
        wait $XVFB_PID 2>/dev/null || true
    fi
    
    echo "Cleanup complete"
}

# Register cleanup function
trap cleanup EXIT INT TERM

# Start recording if requested
if [ "$RECORD" = "true" ]; then
    RECORD_DIR="./build/qemu"
    RECORD_FILE="$RECORD_DIR/screen_record.mp4"
    
    # Create build directory if it doesn't exist, fallback to build/
    if [ ! -d "$RECORD_DIR" ]; then
        RECORD_DIR="./build"
        RECORD_FILE="$RECORD_DIR/screen_record.mp4"
    fi
    
    mkdir -p "$RECORD_DIR"
    
    echo "Starting screen recording to $RECORD_FILE..."
    ffmpeg -video_size ${SCREEN_SIZE%x*} -framerate 30 -f x11grab -i :$DISPLAY_NUM -c:v libx264 -preset ultrafast -crf 23 "$RECORD_FILE" &
    FFMPEG_PID=$!
    
    sleep 1
    
    if ! kill -0 $FFMPEG_PID 2>/dev/null; then
        echo "Warning: ffmpeg failed to start recording"
        FFMPEG_PID=""
    else
        echo "Recording started (PID: $FFMPEG_PID)"
    fi
fi

# Run the appropriate mode
if [ "$RUN_MINUI" = "true" ]; then
    echo ""
    echo "Starting MinUI..."
    echo "Binary: $MINUI_BIN"
    
    # Check if binary exists
    if [ ! -f "$MINUI_BIN" ]; then
        echo "Error: MinUI binary not found at $MINUI_BIN"
        echo "Have you run the builder service to compile MinUI?"
        exit 1
    fi
    
    # Check if it's an ARM ELF binary
    if file "$MINUI_BIN" | grep -q "ARM"; then
        echo "Detected ARM binary"
        
        # Check if qemu-arm-static is available
        if command -v qemu-arm-static &> /dev/null; then
            echo "Using qemu-arm-static to run ARM binary"
            
            # Extract directory for library path
            MINUI_DIR=$(dirname "$MINUI_BIN")
            MINUI_LIB_DIR=$(dirname "$MINUI_DIR")/lib
            
            # Run with qemu
            export LD_LIBRARY_PATH="$MINUI_LIB_DIR:$LD_LIBRARY_PATH"
            exec qemu-arm-static -L "$(dirname "$(dirname "$MINUI_DIR")")" "$MINUI_BIN"
        else
            echo "Warning: ARM binary detected but qemu-arm-static not found"
            echo "Attempting to run natively (may fail)..."
            exec "$MINUI_BIN"
        fi
    else
        # Native binary, run directly
        exec "$MINUI_BIN"
    fi
else
    echo ""
    echo "Starting test runner..."
    
    # Check for test runner (try multiple locations)
    if [ -f "tools/test_runner.py" ]; then
        # Future pattern: tools/test_runner.py --config tests/config.json
        TEST_RUNNER="tools/test_runner.py"
        echo "Test runner: $TEST_RUNNER"
        echo "Tests: $TEST_DIR"
        
        exec python3 "$TEST_RUNNER" --tests "$TEST_DIR" --output "$TEST_OUTPUT"
    elif [ -f "workspace/dev/tools/run_tests.py" ]; then
        # Current pattern: workspace/dev/tools/run_tests.py
        TEST_RUNNER="workspace/dev/tools/run_tests.py"
        echo "Test runner: $TEST_RUNNER"
        echo "Tests: $TEST_DIR"
        echo "Output: $TEST_OUTPUT"
        
        exec python3 "$TEST_RUNNER" --tests "$TEST_DIR" --output "$TEST_OUTPUT"
    else
        echo "Error: Test runner not found"
        echo "Tried:"
        echo "  - tools/test_runner.py"
        echo "  - workspace/dev/tools/run_tests.py"
        exit 1
    fi
fi
