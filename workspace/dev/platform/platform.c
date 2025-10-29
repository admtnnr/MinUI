// dev/platform/platform.c
// Development platform implementation using SDL2

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>

#include "defines.h"
#include "platform.h"
#include "api.h"
#include "utils.h"

// Phase 2 integration
#ifdef USE_CONFIG_SYSTEM
#include "config.h"
#endif

// msettings stubs
#include <msettings.h>

///////////////////////////////
// Platform state

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Surface* screen = NULL;
static int vsync_enabled = 1;
static int window_width = FIXED_WIDTH;
static int window_height = FIXED_HEIGHT;

///////////////////////////////
// Input

void PLAT_initInput(void) {
	// SDL input is initialized with SDL_Init
}

void PLAT_quitInput(void) {
	// SDL input is cleaned up with SDL_Quit
}

void PLAT_pollInput(void) {
	SDL_Event event;
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	// Process events (needed for window management)
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			// User closed the window
			exit(0);
		}
	}

	// Reset transient state
	pad.just_pressed = BTN_NONE;
	pad.just_released = BTN_NONE;
	pad.just_repeated = BTN_NONE;

	// Build current button state from keyboard
	int now_pressed = 0;

	// D-pad
	if (keys[SDL_SCANCODE_UP]) 		now_pressed |= BTN_UP;
	if (keys[SDL_SCANCODE_DOWN]) 	now_pressed |= BTN_DOWN;
	if (keys[SDL_SCANCODE_LEFT]) 	now_pressed |= BTN_LEFT;
	if (keys[SDL_SCANCODE_RIGHT]) 	now_pressed |= BTN_RIGHT;

	// Face buttons
	if (keys[SDL_SCANCODE_X]) 		now_pressed |= BTN_A;
	if (keys[SDL_SCANCODE_Z]) 		now_pressed |= BTN_B;
	if (keys[SDL_SCANCODE_S]) 		now_pressed |= BTN_X;
	if (keys[SDL_SCANCODE_A]) 		now_pressed |= BTN_Y;

	// Shoulder buttons
	if (keys[SDL_SCANCODE_Q]) 		now_pressed |= BTN_L1;
	if (keys[SDL_SCANCODE_W]) 		now_pressed |= BTN_R1;
	if (keys[SDL_SCANCODE_E]) 		now_pressed |= BTN_L2;
	if (keys[SDL_SCANCODE_R]) 		now_pressed |= BTN_R2;
	if (keys[SDL_SCANCODE_T]) 		now_pressed |= BTN_L3;
	if (keys[SDL_SCANCODE_Y]) 		now_pressed |= BTN_R3;

	// System buttons
	if (keys[SDL_SCANCODE_RSHIFT]) 	now_pressed |= BTN_SELECT;
	if (keys[SDL_SCANCODE_RETURN]) 	now_pressed |= BTN_START;
	if (keys[SDL_SCANCODE_ESCAPE]) 	now_pressed |= BTN_MENU;
	if (keys[SDL_SCANCODE_P]) 		now_pressed |= BTN_POWER;
	if (keys[SDL_SCANCODE_EQUALS]) 	now_pressed |= BTN_PLUS;
	if (keys[SDL_SCANCODE_MINUS]) 	now_pressed |= BTN_MINUS;

	// Calculate just_pressed and just_released
	pad.just_pressed = now_pressed & ~pad.is_pressed;  // Buttons that are now pressed but weren't before
	pad.just_released = ~now_pressed & pad.is_pressed; // Buttons that were pressed but aren't now

	// Update current state
	pad.is_pressed = now_pressed;

	// TODO Phase 3.3: Add joystick/gamepad support
	// TODO: Handle button repeat logic
}

int PLAT_shouldWake(void) {
	// For dev platform, we don't have sleep/wake
	return 0;
}

///////////////////////////////
// Video

SDL_Surface* PLAT_initVideo(void) {
	// Load configuration if available
#ifdef USE_CONFIG_SYSTEM
	minui_config_t* config = config_load(NULL);
	if (config) {
		CONFIG_set(config);
		if (DEBUG_enabled()) {
			LOG_info("dev platform: Loaded configuration\n");
		}
	}
#endif

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		LOG_error("SDL_Init failed: %s\n", SDL_GetError());
		exit(1);
	}

	// Create window
	window = SDL_CreateWindow(
		"MinUI Dev Platform",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_width,
		window_height,
		SDL_WINDOW_SHOWN
	);

	if (!window) {
		LOG_error("SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	// Create renderer with vsync if enabled
	Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
	if (vsync_enabled) {
		renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
	}

	renderer = SDL_CreateRenderer(window, -1, renderer_flags);
	if (!renderer) {
		LOG_error("SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}

	// Set default render scale quality
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest neighbor by default

	// Create screen surface
	screen = SDL_CreateRGBSurface(
		0,
		FIXED_WIDTH,
		FIXED_HEIGHT,
		FIXED_DEPTH,
		0x00FF0000,  // R mask
		0x0000FF00,  // G mask
		0x000000FF,  // B mask
		0xFF000000   // A mask
	);

	if (!screen) {
		LOG_error("SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}

	// Create texture for rendering
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		FIXED_WIDTH,
		FIXED_HEIGHT
	);

	if (!texture) {
		LOG_error("SDL_CreateTexture failed: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}

	// Create SDCARD_PATH directory if it doesn't exist
	mkdir(SDCARD_PATH, 0755);

	LOG_info("dev platform: Video initialized (%dx%d, vsync=%d)\n",
		window_width, window_height, vsync_enabled);

	return screen;
}

SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch) {
	// For dev platform, we don't dynamically resize
	// Just return the existing screen surface
	// TODO Phase 3.2: Implement dynamic resizing
	(void)w;
	(void)h;
	(void)pitch;
	return screen;
}

void PLAT_quitVideo(void) {
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = NULL;
	}

	if (screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}

	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	SDL_Quit();

	LOG_info("dev platform: Video quit\n");
}

void PLAT_clearVideo(SDL_Surface* surface) {
	if (!surface) return;
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
}

void PLAT_clearAll(void) {
	if (renderer) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}
}

void PLAT_setVsync(int vsync) {
	vsync_enabled = vsync;
	LOG_info("dev platform: VSync set to %d (requires restart to apply)\n", vsync);
}

void PLAT_setVideoScaleClip(int x, int y, int width, int height) {
	// TODO Phase 3.2: Implement viewport clipping
}

void PLAT_setNearestNeighbor(int enabled) {
	if (enabled) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	} else {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	}
}

void PLAT_setSharpness(int sharpness) {
	// TODO Phase 3.2: Implement sharpness shader
}

void PLAT_setEffectColor(int color) {
	// TODO Phase 3.2: Implement color effects
}

void PLAT_setEffect(int effect) {
	// TODO Phase 3.2: Implement display effects
}

void PLAT_vsync(int remaining) {
	// SDL handles vsync through renderer flags
	if (remaining > 0) {
		SDL_Delay(remaining);
	}
}

void PLAT_blitRenderer(GFX_Renderer* gfx_renderer) {
	// TODO: Implement if needed for advanced rendering
	(void)gfx_renderer;
}

scaler_t PLAT_getScaler(GFX_Renderer* renderer) {
	// For dev platform, return NULL - SDL handles scaling
	// TODO Phase 3.2: Implement software scalers if needed
	(void)renderer;
	return NULL;
}

void PLAT_flip(SDL_Surface* surface, int sync) {
	if (!surface || !renderer || !texture) return;

	// Update texture from surface
	SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);

	// Render texture to screen
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int PLAT_supportsOverscan(void) {
	return 0; // No overscan on desktop platforms
}

///////////////////////////////
// Overlay (not used in dev platform)

SDL_Surface* PLAT_initOverlay(void) {
	// Not implemented for dev platform
	return NULL;
}

void PLAT_quitOverlay(void) {
	// Not implemented
}

void PLAT_enableOverlay(int enable) {
	// Not implemented
	(void)enable;
}

///////////////////////////////
// Lid (not used in dev platform)

void PLAT_initLid(void) {
	// Not implemented
}

int PLAT_lidChanged(int* state) {
	return 0; // No lid on dev platform
}

///////////////////////////////
// Battery and Power

void PLAT_getBatteryStatus(int* is_charging, int* charge) {
	// Simulate full battery on AC power for dev platform
	*is_charging = 1;
	*charge = 100;
}

void PLAT_enableBacklight(int enable) {
	// Not applicable for dev platform
}

void PLAT_powerOff(void) {
	// Clean shutdown
	PLAT_quitVideo();
	exit(0);
}

///////////////////////////////
// CPU and Performance

void PLAT_setCPUSpeed(int speed) {
	// Not applicable for dev platform
	// CPU speed is controlled by OS
}

void PLAT_setRumble(int strength) {
	// TODO Phase 3.3: Implement with SDL haptic feedback if available
}

///////////////////////////////
// Audio

int PLAT_pickSampleRate(int requested, int max) {
	// Standard sample rates for dev platform
	int rates[] = {48000, 44100, 32000, 22050, 16000, 11025, 8000};

	for (int i = 0; i < 7; i++) {
		if (rates[i] <= requested && rates[i] <= max) {
			return rates[i];
		}
	}

	return 44100; // Default fallback
}

///////////////////////////////
// Utility

char* PLAT_getModel(void) {
	return "dev";
}

int PLAT_isOnline(void) {
	// Dev platform is always "online" (has network access)
	return 1;
}

// PLAT_setDateTime has a fallback implementation in api.c

///////////////////////////////
// msettings stubs

void InitSettings(void) {
	// Not needed for dev platform
}

void QuitSettings(void) {
	// Not needed for dev platform
}

int GetBrightness(void) {
	return 10; // Max brightness
}

int GetVolume(void) {
	return 20; // Max volume
}

void SetRawBrightness(int value) {
	// Not implemented for dev platform
	(void)value;
}

void SetRawVolume(int value) {
	// Not implemented for dev platform
	(void)value;
}

void SetBrightness(int value) {
	// Not implemented for dev platform
	(void)value;
}

void SetVolume(int value) {
	// Not implemented for dev platform
	(void)value;
}

int GetJack(void) {
	return 0; // No headphone jack
}

void SetJack(int value) {
	// Not implemented for dev platform
	(void)value;
}

int GetHDMI(void) {
	return 0; // No HDMI
}

void SetHDMI(int value) {
	// Not implemented for dev platform
	(void)value;
}

int GetMute(void) {
	return 0; // Not muted
}

///////////////////////////////
