# MinUI Docker Development Environment

This containerized development environment provides a complete setup for running MinUI tests visually and debugging MinUI on macOS (M1/ARM64) and other platforms.

## Features

- **Visual Testing**: Xvfb with VNC access to watch test runs in real-time
- **ARM Binary Support**: QEMU user-mode emulation for running ARM binaries on any platform
- **Screen Recording**: Optional ffmpeg-based recording of test sessions
- **Dual Runner Modes**: Run automated tests or launch MinUI directly for debugging
- **Shared Build Artifacts**: Builder service compiles once, runner reuses artifacts
- **Configurable Screen Size**: Auto-detects from platform files or manually override

## Quick Start

### 1. Build the Docker Image

```bash
make docker-build
```

This creates the `minui-dev:latest` image based on Ubuntu 22.04 ARM64 with all necessary tools.

### 2. Compile MinUI (Builder Service)

```bash
make docker-build-minui
```

The builder service compiles MinUI and stores artifacts in a shared Docker volume named `minui-build-artifacts`. You can customize the compile command:

```bash
COMPILE_CMD="cd workspace/dev && make clean && make" make docker-build-minui
```

### 3. Run Tests with VNC

```bash
make docker-test
```

This starts the runner service with:
- Xvfb running on display :99
- x11vnc server on port 5900 (no password)
- Python test runner executing tests

**Connect via VNC**: Use any VNC client to connect to `localhost:5900` (no password required).

### 4. Debug MinUI Directly

```bash
make docker-debug
```

Runs MinUI binary directly with VNC access for interactive debugging.

## Architecture

### Services

#### Builder Service
- **Purpose**: Compiles MinUI and dependencies
- **Output**: Build artifacts stored in `minui-build-artifacts` Docker volume
- **Usage**: One-shot compilation, run when code changes
- **Customization**: Set `COMPILE_CMD` environment variable

#### Runner Service
- **Purpose**: Runs tests or MinUI with visual output
- **Display**: Xvfb + x11vnc for VNC access
- **Modes**: 
  - Test runner mode (default): Executes automated Python tests
  - MinUI mode: Launches MinUI binary directly
- **VNC Port**: 5900 (configurable via `VNC_PORT`)

### Volume Sharing

```
┌─────────────┐
│   Builder   │
│   Service   │──────┐
└─────────────┘      │
                     ▼
              ┌─────────────────┐
              │ Build Artifacts │  (Docker Volume)
              │     Volume      │
              └─────────────────┘
                     ▲
┌─────────────┐      │
│   Runner    │      │
│   Service   │──────┘
└─────────────┘
```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SCREEN` | `640x480x24` | Screen resolution (WIDTHxHEIGHTxDEPTH) |
| `RUN_MINUI` | `false` | Set to `true` to run MinUI instead of tests |
| `RECORD` | `false` | Enable screen recording to MP4 |
| `TEST_DIR` | `workspace/dev/tests` | Test directory path |
| `TEST_OUTPUT` | `workspace/dev/test_output` | Test output directory path |
| `MINUI_BIN` | `./build/SYSTEM/rg35xx/bin/minui.elf` | Path to MinUI binary |
| `DISPLAY_NUM` | `99` | X display number |
| `VNC_PORT` | `5900` | VNC server port |
| `COMPILE_CMD` | `cd workspace/dev && make` | Command to run in builder service |

### Screen Size Detection

The entrypoint automatically detects screen size from platform files:

1. **Priority Order**:
   - CLI flag `--screen SIZE`
   - Environment variable `SCREEN`
   - Auto-detected from `workspace/dev/platform/platform.h`
   - Default: `640x480x24`

2. **Detection Logic**:
   - Reads `FIXED_WIDTH`, `FIXED_HEIGHT`, and `FIXED_DEPTH` from platform.h
   - Constructs screen size as `WIDTHxHEIGHTxDEPTH`

## Usage Examples

### Basic Test Run

```bash
# Build image and compile MinUI
make docker-build
make docker-build-minui

# Run tests with VNC
make docker-test
```

Connect to `localhost:5900` with a VNC client to watch tests.

### Custom Screen Size

```bash
docker compose run --rm -e SCREEN=800x600x24 runner
```

### Run Tests from Different Directory

```bash
docker compose run --rm -e TEST_DIR=workspace/dev/tests -e TEST_OUTPUT=./test_results runner
```

### Debug MinUI with Recording

```bash
docker compose run --rm -e RUN_MINUI=true -e RECORD=true runner
```

The recording is saved to `build/qemu/screen_record.mp4` or `build/screen_record.mp4`.

### Interactive Shell in Runner

```bash
docker compose run --rm runner bash
```

This drops you into a bash shell with Xvfb and VNC running in the background.

### Using Docker Compose Directly

```bash
# Start runner in background
docker compose up -d runner

# View logs
docker compose logs -f runner

# Stop runner
docker compose down
```

### Cleanup

```bash
# Remove containers and volumes
make docker-clean

# Or manually
docker compose down -v
docker volume rm minui-build-artifacts
```

## Advanced Usage

### Custom Compile Command

Compile a specific platform or with custom flags:

```bash
COMPILE_CMD="cd workspace/all/minui && PLATFORM=dev make clean && make" \
  docker compose --profile build run --rm builder
```

### Multiple VNC Sessions

Run multiple runners on different ports:

```bash
# Terminal 1
docker compose run --rm -e VNC_PORT=5901 -p 5901:5901 runner

# Terminal 2
docker compose run --rm -e VNC_PORT=5902 -p 5902:5902 runner
```

### ARM Binary Execution

When `RUN_MINUI=true` and the binary is ARM ELF:
- The entrypoint automatically detects ARM architecture
- Uses `qemu-arm-static` for emulation
- Sets appropriate library paths

Example for custom ARM binary:

```bash
docker compose run --rm \
  -e RUN_MINUI=true \
  -e MINUI_BIN=./build/SYSTEM/miyoomini/bin/minui.elf \
  runner
```

## Troubleshooting

### VNC Connection Issues

**Problem**: Can't connect to VNC

**Solutions**:
1. Check if port 5900 is available: `lsof -i :5900`
2. Try a different port: `-e VNC_PORT=5901 -p 5901:5901`
3. Check container logs: `docker compose logs runner`

### Binary Not Found

**Problem**: `Error: MinUI binary not found`

**Solution**: Run the builder service first:
```bash
make docker-build-minui
```

### Screen Size Issues

**Problem**: Wrong screen resolution

**Solutions**:
1. Override with environment variable: `-e SCREEN=640x480x24`
2. Use command line flag: `docker compose run --rm runner --screen 640x480x24`
3. Check platform.h for correct values

### Recording Fails

**Problem**: ffmpeg recording doesn't start

**Solutions**:
1. Check ffmpeg is installed in image: `docker compose run --rm runner which ffmpeg`
2. Ensure build directory exists: `mkdir -p build/qemu`
3. Check logs for ffmpeg errors

### QEMU Issues with ARM Binaries

**Problem**: ARM binary won't run

**Solutions**:
1. Check qemu is installed: `docker compose run --rm runner which qemu-arm-static`
2. Verify binary architecture: `docker compose run --rm runner file ./build/SYSTEM/rg35xx/bin/minui.elf`
3. Check library paths are correct

## Development Workflow

### Typical Development Cycle

```bash
# 1. Initial setup
make docker-build

# 2. Compile MinUI
make docker-build-minui

# 3. Run tests
make docker-test
# Connect via VNC to watch

# 4. Make code changes in workspace/

# 5. Rebuild
make docker-build-minui

# 6. Test again
make docker-test

# 7. Debug if needed
make docker-debug
```

### CI/CD Integration

For automated testing without VNC:

```bash
# Headless test run (no VNC client needed)
docker compose run --rm runner bash -c "
  cd workspace/dev/tools && 
  python3 run_tests.py --headless
"
```

## Technical Details

### Dockerfile

- **Base**: `ubuntu:22.04` (ARM64)
- **Packages**: xvfb, x11vnc, ffmpeg, python3, build-essential, qemu-user-static
- **User**: Non-root `dev` user (UID varies)
- **Working Directory**: `/work`
- **Entrypoint**: `/usr/local/bin/dev-entrypoint.sh`

### Entrypoint Script

Location: `scripts/dev-entrypoint.sh`

**Responsibilities**:
1. Parse arguments and environment variables
2. Detect or set screen resolution
3. Start Xvfb on specified display
4. Start x11vnc on specified port
5. Optionally start ffmpeg recording
6. Run test runner or MinUI binary
7. Cleanup on exit (stop Xvfb, VNC, recording)

**Signals Handled**:
- `INT` (Ctrl+C)
- `TERM` (docker stop)
- `EXIT` (any exit)

### Volume Structure

```
minui-build-artifacts/
├── SYSTEM/
│   └── rg35xx/
│       ├── bin/
│       │   └── minui.elf
│       └── lib/
│           └── libmsettings.so
└── qemu/
    └── screen_record.mp4  (if recording enabled)
```

## Platform Support

This Docker environment is designed for:
- **macOS M1/M2** (ARM64): Native ARM execution
- **Linux ARM64**: Native ARM execution
- **Linux x86_64**: Uses QEMU for ARM binaries
- **macOS Intel**: Uses QEMU for ARM binaries (slower)

## License

Same as MinUI project license.
