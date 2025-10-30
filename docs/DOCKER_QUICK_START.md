# Docker Development Environment - Quick Start

This guide will get you up and running with the MinUI Docker development environment in 5 minutes.

## Prerequisites

- Docker and Docker Compose installed
- VNC client (e.g., RealVNC Viewer, TightVNC, macOS Screen Sharing)
- At least 2GB free disk space

## Step 1: Build the Docker Image (1 minute)

```bash
make docker-build
```

This creates the `minui-dev:latest` image with all necessary tools.

## Step 2: Compile MinUI (2-5 minutes)

```bash
make docker-build-minui
```

This runs the builder service to compile MinUI and store artifacts in a Docker volume.

**Note**: First build may take a few minutes to compile everything. Subsequent builds are faster.

## Step 3: Run Tests with Visual Output (30 seconds)

```bash
make docker-test
```

This starts the runner with:
- Xvfb (virtual display)
- x11vnc server on port 5900
- Python test runner

**Connect via VNC**:
1. Open your VNC client
2. Connect to `localhost:5900`
3. No password required
4. You should see the MinUI test runner executing tests

## Step 4: Debug MinUI Interactively

To run MinUI directly for debugging:

```bash
make docker-debug
```

Connect via VNC to see MinUI running in the virtual display.

## Troubleshooting

### Can't connect to VNC?

Check if the runner is running:
```bash
docker ps
```

View logs:
```bash
docker compose logs runner
```

### Port 5900 already in use?

Use a different port:
```bash
docker compose run --rm -e VNC_PORT=5901 -p 5901:5901 runner
```

Then connect to `localhost:5901`.

### Binary not found?

Make sure you ran the builder:
```bash
make docker-build-minui
```

Check the build artifacts:
```bash
docker run --rm -v minui-build-artifacts:/work/build alpine ls -la /work/build
```

## Advanced Usage

### Custom Screen Size

```bash
docker compose run --rm -e SCREEN=800x600x24 runner
```

### Record Test Session

```bash
docker compose run --rm -e RECORD=true runner
```

Video saved to `build/qemu/screen_record.mp4` or `build/screen_record.mp4`.

### Interactive Shell

Get a shell in the runner environment:

```bash
docker compose run --rm runner bash
```

Inside the container:
```bash
# Check display
echo $DISPLAY

# List build artifacts
ls -la build/

# Run test manually
python3 workspace/dev/tools/run_tests.py --tests workspace/dev/tests
```

### Custom Compile Command

```bash
COMPILE_CMD="cd workspace/all/minui && PLATFORM=dev make clean && make" \
  docker compose --profile build run --rm builder
```

## Cleanup

Remove all Docker artifacts:

```bash
make docker-clean
```

This removes:
- Docker containers
- Build artifacts volume
- Cached images (use `docker image prune` separately if desired)

## Next Steps

- Read the full documentation: [DOCKER_DEV_ENVIRONMENT.md](DOCKER_DEV_ENVIRONMENT.md)
- Explore test configurations in `workspace/dev/tests/`
- Modify `workspace/dev/tools/run_tests.py` to customize test execution
- Create your own test scenarios

## Common Workflows

### Development Cycle

```bash
# 1. Make code changes in workspace/

# 2. Rebuild
make docker-build-minui

# 3. Test
make docker-test

# 4. Debug if needed
make docker-debug
```

### CI/CD Testing

```bash
# Run tests headless (no VNC needed)
docker compose run --rm runner bash -c \
  "python3 workspace/dev/tools/run_tests.py --tests workspace/dev/tests --headless"
```

### Multi-Platform Testing

```bash
# Build for different platform
COMPILE_CMD="cd workspace/rg35xx && make" \
  docker compose --profile build run --rm builder

# Run MinUI for that platform
docker compose run --rm \
  -e RUN_MINUI=true \
  -e MINUI_BIN=./build/SYSTEM/rg35xx/bin/minui.elf \
  runner
```

## Tips

1. **Keep VNC connected**: Leave VNC client open while running multiple tests to watch progress
2. **Use recordings**: Enable recording for tests that fail intermittently
3. **Check logs**: Always check `docker compose logs` if something doesn't work
4. **Rebuild after changes**: The builder service only runs once, rebuild after code changes
5. **Volume persistence**: Build artifacts persist in Docker volume between runs

## Support

For issues or questions:
- Check the full documentation
- Review entrypoint script: `scripts/dev-entrypoint.sh`
- Check Docker Compose logs: `docker compose logs -f`
- Verify system requirements (Docker version, disk space, etc.)
