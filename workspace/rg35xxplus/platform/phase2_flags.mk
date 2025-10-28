# RG35XX Plus Feature Flags
# Enable these flags to test Phase 2 components
# Set to 1 to enable, 0 to disable

# Configuration system support
# When enabled, loads settings from /mnt/sdcard/.userdata/minui.conf
USE_CONFIG_SYSTEM ?= 1

# Graphics backend abstraction
# When enabled, allows selecting graphics backend via config
# Default: keeps existing hardware-accelerated path (ION + Display Engine)
USE_GFX_BACKEND ?= 0

# Framebuffer backend fallback
# When enabled, compiles fbdev backend as alternative to hardware path
# Only useful if USE_GFX_BACKEND=1
USE_FBDEV_BACKEND ?= 0

# Frame queue for threaded video
# When enabled, allows thread_video=1 in config
# Experimental: may improve performance on quad-core Cortex-A53
USE_FRAME_QUEUE ?= 0

# Debug logging for Phase 2 integration
# When enabled, adds verbose logging for new systems
DEBUG_PHASE2 ?= 0

# Export flags for use in makefiles
export USE_CONFIG_SYSTEM
export USE_GFX_BACKEND
export USE_FBDEV_BACKEND
export USE_FRAME_QUEUE
export DEBUG_PHASE2
