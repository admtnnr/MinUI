# Dev platform cores
# Builds native libretro cores for development and testing
# Supports both x86_64 (CI) and arm64 (M1 Mac Docker)

# Start with just gambatte for Phase 3.5
CORES = gambatte

###############################

# Core-specific overrides
gambatte_REPO = https://github.com/libretro/gambatte-libretro

###############################

# For dev platform, we build native cores without cross-compilation
# Override the MAKE command to build for host architecture
define CORE_MAKE_OVERRIDE
$1_MAKE ?= make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) $($(1)_FLAGS)
endef

$(foreach CORE,$(CORES),$(eval $(call CORE_MAKE_OVERRIDE,$(CORE))))

###############################

include ../../all/cores/makefile
