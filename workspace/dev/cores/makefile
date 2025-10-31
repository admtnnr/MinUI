# Dev platform cores
# Builds native libretro cores for development and testing
# Supports both x86_64 (CI) and arm64 (M1 Mac Docker)

# Phase 3.6: All supported cores
CORES = fceumm gambatte gpsp picodrive snes9x2005_plus
CORES+= mednafen_pce_fast mednafen_vb fake-08 mgba race pcsx_rearmed

# Optional: Add these manually if needed (have build issues)
# pokemini - Has patching issues on dev platform
# mednafen_supafaust - SNES alternative

###############################

# Core-specific repository overrides
mednafen_pce_fast_REPO = https://github.com/libretro/beetle-pce-fast-libretro
mednafen_vb_REPO = https://github.com/libretro/beetle-vb-libretro

fake-08_REPO = https://github.com/jtothebell/fake-08
fake-08_CORE = fake08_libretro.so
fake-08_BUILD_PATH = fake-08/platform/libretro

fceumm_REPO = https://github.com/libretro/libretro-fceumm

gambatte_REPO = https://github.com/libretro/gambatte-libretro

picodrive_REPO = https://github.com/irixxxx/picodrive
picodrive_MAKEFILE = Makefile.libretro

pcsx_rearmed_MAKEFILE = Makefile.libretro
# For pcsx_rearmed, we need to patch .gitmodules before initializing submodules
# so we override the default clone behavior to not use --recursive
pcsx_rearmed_NEEDS_SUBMODULE_PATCH = 1

pokemini_REPO = https://github.com/libretro/PokeMini
pokemini_MAKEFILE = Makefile.libretro

snes9x2005_plus_REPO = https://github.com/libretro/snes9x2005
snes9x2005_plus_FLAGS = USE_BLARGG_APU=1

###############################

# For dev platform, we build native cores without cross-compilation
# Override the MAKE command to build for host architecture
define CORE_MAKE_OVERRIDE
$1_MAKE ?= make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) $($(1)_FLAGS)
endef

$(foreach CORE,$(CORES),$(eval $(call CORE_MAKE_OVERRIDE,$(CORE))))

###############################

include ../../all/cores/makefile

###############################

# Custom clone rule for pcsx_rearmed to patch .gitmodules before submodule init
# This must come after the include to override the default clone rule
src/pcsx_rearmed:
	mkdir -p src
	cd src && git clone --depth 1 https://github.com/libretro/pcsx_rearmed pcsx_rearmed
	cd src/pcsx_rearmed && sed -i 's|git://git.sv.gnu.org/gnulib.git|https://github.com/coreutils/gnulib.git|g' .gitmodules
	cd src/pcsx_rearmed && git submodule update --init --recursive
