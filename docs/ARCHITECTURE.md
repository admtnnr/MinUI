# MinUI Architecture and Build Overview

This document summarizes how MinUI is organized on disk and how the build
system assembles firmware images for supported handhelds. It is intended as
background reading before extending the project with new backends or
platform targets.

## Repository Layout

The root of the repository is split between assets that describe the runtime
filesystem shipped to devices and the cross-compiled software that runs on
those filesystems:

| Path | Description |
| ---- | ----------- |
| `skeleton/` | Golden copy of the SD card layout that is copied into `build/` at the start of a release. Contains the `BASE/`, `SYSTEM/`, and `EXTRAS/` trees that ship to users. |
| `workspace/` | Source code and build logic for MinUI executables, libretro cores, and platform-specific tooling. This directory is mounted into the cross-compilation container during builds. |
| `makefile` | Host-side entry point for assembling a release bundle. Handles staging the `skeleton/`, invoking workspace builds for each platform, and packaging `.zip` payloads. |
| `makefile.toolchain` | Utility makefile that fetches the per-platform Docker toolchains and launches an interactive or batch build inside them. |
| `releases/` | Populated during packaging. Each `make` run produces `MinUI-<date>-base.zip` and `MinUI-<date>-extras.zip` archives here. |

The `build/` directory is ephemeral and recreated for every release. It
starts as a copy of `skeleton/` and is gradually populated with freshly built
binaries, cores, and metadata before packaging.

## Workspace Structure

The `workspace/` directory is the root of all cross-compiled components. It
contains both platform-agnostic projects under `workspace/all/` and folders
per hardware target (for example `workspace/rg35xx/`).

### Shared Projects (`workspace/all/`)

* `minui/` – Main launcher responsible for browsing the SD card and starting
  libretro cores.
* `minarch/` – Libretro frontend that provides consistent in-game menus and
  emulator controls.
* `minput/`, `clock/`, `say/`, `syncsettings/` – Small companion utilities
  that ship in the `EXTRAS` tree.
* `common/` – Shared code (`api.c`, `scaler.c`, etc.) consumed by MinUI and
  Minarch.
* `cores/` – Platform-independent wrapper makefile used by per-platform
  builds to fetch and compile libretro cores.

Each project exposes a makefile that expects a `PLATFORM` name and inherits
compiler settings from `workspace/<platform>/platform/makefile.env`. The
makefiles compile into `workspace/all/<project>/build/<platform>/` and rely on
libraries like `libmsettings` that are built by the platform-specific portion
of the workspace.

### Platform-Specific Folders

Every platform folder (e.g. `workspace/rg35xx/`) includes:

* `makefile` – Top-level orchestration for platform assets. It builds helper
  apps (such as DinguxCommander), calls into `boot/` scripts, and formats
  README files via `workspace/all/readmes`.
* `platform/` – Defines compiler flags (`makefile.env`) and provides helper
  sources (e.g. `platform.c`) that are linked into MinUI and Minarch.
* `libmsettings/`, `keymon/`, `cores/`, etc. – Platform-specific toolchains
  and libretro core builds.
* Optional extras such as `overclock/`, `install/`, or `ramdisk/` that are
  staged into the SD card image during packaging.

The workspace root also includes `makefile`, which is executed inside the
cross-compilation container. It builds shared libraries, invokes each shared
project, compiles per-platform cores, and finally runs the platform `make` to
assemble staging artifacts.

## Build Pipeline

1. **Toolchain initialization** – `make PLATFORM=<name> build` on the host
   clones the platform-specific Docker toolchain (`union-<platform>-toolchain`)
   if necessary and runs it via `docker run`. The repository `workspace/`
   directory is bind-mounted as `/root/workspace` inside the container.
2. **Workspace build** – Inside the container, `make` executes
   `workspace/makefile`. The shared projects and platform-specific components
   compile using the cross-compiler provided by the toolchain. Outputs land in
   the mounted `workspace/` tree.
3. **Staging** – Back on the host, the top-level `make` target copies the
   `skeleton/` into `build/`, places built binaries under `build/SYSTEM/<platform>/`
   and `build/EXTRAS/...`, and performs special-case transformations (for
   example moving `.tmp_update` payloads for the Miyoo family).
4. **Packaging** – The `package` target zips the populated `BASE/`, `SYSTEM/`,
   and `EXTRAS/` trees into release archives and writes metadata such as
   `version.txt` and `commits.txt`.

The `setup`, `system`, `cores`, `special`, and `tidy` targets defined in the
root `makefile` orchestrate the staging details for each supported platform.
They ensure that fresh binaries from the workspace build are copied to the
correct SD card paths before zipping.

## Development Workflow

* To enter an interactive toolchain shell for a platform:

  ```sh
  make PLATFORM=<platform> shell
  ```

* To perform a full rebuild for a single platform and stage it into the
  release structure:

  ```sh
  make PLATFORM=<platform> build
  make PLATFORM=<platform> system cores
  ```

  Subsequent invocations of `make` at the repository root can target multiple
  platforms by setting the `PLATFORMS` variable or using the default list in
  the makefile.

Understanding these layers—`skeleton/` for filesystem templates,
`workspace/` for cross-compiled outputs, and the root makefiles for packaging—
provides the foundation needed to add new backend targets such as a QEMU-based
emulator configuration in future work.
