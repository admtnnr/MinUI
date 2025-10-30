# Docker Development Environment

This document describes how to use the Docker-based development environment for MinUI, which provides a containerized workflow with visual display capabilities via VNC.

## Overview

The Docker environment provides:
- **Visual Display**: X11 forwarding via VNC on port 5900
- **Test Runner**: Automated test execution with visual feedback
- **Screen Recording**: Optional ffmpeg recording of test sessions
- **Build Isolation**: Consistent build environment across platforms

## Prerequisites

- Docker Desktop (for macOS/Windows) or Docker Engine (for Linux)
- Docker Compose
- VNC viewer (optional, for connecting to the visual display)

### macOS M1/M2 Notes

For Apple Silicon Macs, you may need to configure Docker buildx for multi-platform builds:

```bash
# Create a new builder instance
docker buildx create --name minui-builder --use

# Verify builder supports arm64
docker buildx inspect --bootstrap
```

The `docker-compose.yml` is configured with `platform: linux/arm64` for ARM compatibility, but can be changed to `linux/amd64` if needed.

## Quick Start

### 1. Build Docker Images

```bash
make dev-docker-build
```

Or manually:

```bash
docker-compose build --parallel
```

### 2. Build MinUI (Optional)

To compile MinUI inside the container:

```bash
make dev-docker-builder
```

Or with custom compile command:

```bash
COMPILE_CMD="make PLATFORM=dev build" make dev-docker-builder
```

### 3. Run MinUI with Visual Display

```bash
make dev-docker-run
```

This will:
- Start Xvfb (virtual X display)
- Start x11vnc server on port 5900
- Start MinUI automatically
- Run the test runner (by default)
- Keep the container alive for inspection

**Note**: MinUI always starts automatically. The test runner is optional.

### 4. Connect to VNC Display

While the container is running, connect with a VNC viewer:

```bash
# macOS
open vnc://localhost:5900

# Linux (with vncviewer)
vncviewer localhost:5900

# Or use any VNC client
```

No password is required for the VNC connection.

## Advanced Usage

### Run MinUI Without Tests

MinUI always starts automatically. To run it without tests (for manual interaction):

```bash
docker-compose run --rm -p 5900:5900 runner
```

This starts MinUI and keeps it running for VNC inspection.

### Run MinUI With Tests

To explicitly run tests (this is the default behavior with `make dev-docker-run`):

```bash
docker-compose run --rm -p 5900:5900 runner --run-tests
```

### Custom Screen Resolution

```bash
docker-compose run --rm -p 5900:5900 runner --screen 800x600x24 --run-tests
```

### Enable Screen Recording

```bash
docker-compose run --rm -p 5900:5900 runner --record --run-tests
```

Recordings are saved to `./build/recordings/screen_TIMESTAMP.mp4`.

### Custom Test Configuration

Set environment variables:

```bash
MINUI_BIN=/custom/path/to/minui.elf docker-compose run --rm runner --run-tests
```

### Interactive Shell

To get a shell inside the container:

```bash
docker-compose run --rm --entrypoint /bin/bash runner
```

## Environment Variables

The entrypoint script supports the following environment variables:

- `SCREEN`: Screen resolution (e.g., "640x480x24"), auto-detected from makefile.env if not set
- `RECORD_VIDEO`: Set to "true" to enable recording (same as --record flag)
- `MINUI_BIN`: Path to MinUI binary (default: `/work/build/SYSTEM/dev/bin/minui.elf`)
- `COMPILE_CMD`: Custom compilation command for builder service

**Note**: `RUN_MINUI` has been removed. MinUI now always starts automatically. Use `--run-tests` flag to optionally run tests.

## Makefile Targets

The top-level Makefile includes convenience targets:

- `make dev-docker-build`: Build Docker images
- `make dev-docker-builder`: Run builder container to compile MinUI
- `make dev-docker-run`: Run MinUI with test suite and visual display

## Troubleshooting

### VNC Connection Refused

Make sure the container is running and port 5900 is not blocked:

```bash
docker-compose ps
netstat -an | grep 5900
```

### MinUI Binary Not Found

Build MinUI first:

```bash
make dev-docker-builder COMPILE_CMD="make PLATFORM=dev build"
```

### Screen Resolution Issues

The entrypoint automatically detects screen resolution from `workspace/dev/platform/makefile.env`. You can override with:

```bash
docker-compose run --rm runner --screen 640x480x24
```

### Build Platform Issues (macOS M1)

If you encounter platform mismatch errors, ensure Docker Desktop is configured for the correct architecture:

1. Check Docker Desktop Settings > General > "Use Rosetta for x86/amd64 emulation"
2. Or modify `docker-compose.yml` to use `platform: linux/amd64`

## File Structure

```
.
├── Dockerfile                    # Container image definition
├── docker-compose.yml            # Service orchestration
├── .dockerignore                 # Files excluded from image
├── scripts/
│   └── dev-entrypoint.sh        # Container entrypoint script
└── docs/
    └── DEV_DOCKER.md            # This file
```

## Integration with CI/CD

The Docker environment can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Actions workflow
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build images
        run: make dev-docker-build
      - name: Run tests
        run: docker-compose up --abort-on-container-exit runner
```

## Next Steps

- Extend test suite in `workspace/dev/tests/`
- Add custom test configurations
- Configure automated screenshots and recordings
- Integrate with CI/CD pipeline

For more information on the test framework, see `workspace/dev/tools/README.md`.
