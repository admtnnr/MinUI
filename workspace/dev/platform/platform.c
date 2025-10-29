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

// Phase 3.2: Platform configuration
#include "platform_config.h"
#include "platform_config.c"  // Include implementation directly (simpler than modifying build system)

// msettings stubs
#include <msettings.h>

///////////////////////////////
// Platform state

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_Surface* screen = NULL;
static platform_config_t platform_config;
static int window_width = FIXED_WIDTH;
static int window_height = FIXED_HEIGHT;

// Joystick/Gamepad state
static SDL_GameController* gamepad = NULL;
static SDL_Joystick* joystick = NULL;
static int joystick_id = -1;

///////////////////////////////
// Input

static void openGamepad(int device_index) {
	// Close existing gamepad if any
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = NULL;
	}
	if (joystick) {
		SDL_JoystickClose(joystick);
		joystick = NULL;
	}

	// Try to open as game controller first (preferred, has standard mapping)
	if (SDL_IsGameController(device_index)) {
		gamepad = SDL_GameControllerOpen(device_index);
		if (gamepad) {
			joystick = SDL_GameControllerGetJoystick(gamepad);
			joystick_id = SDL_JoystickInstanceID(joystick);
			LOG_info("dev platform: Opened gamepad: %s\n",
				SDL_GameControllerName(gamepad));
			return;
		}
	}

	// Fall back to joystick API
	joystick = SDL_JoystickOpen(device_index);
	if (joystick) {
		joystick_id = SDL_JoystickInstanceID(joystick);
		LOG_info("dev platform: Opened joystick: %s (%d axes, %d buttons)\n",
			SDL_JoystickName(joystick),
			SDL_JoystickNumAxes(joystick),
			SDL_JoystickNumButtons(joystick));
	}
}

void PLAT_initInput(void) {
	// Enumerate and open the first available controller
	int num_joysticks = SDL_NumJoysticks();

	if (num_joysticks > 0) {
		LOG_info("dev platform: Found %d joystick(s)\n", num_joysticks);
		openGamepad(0); // Open first controller
	} else {
		LOG_info("dev platform: No joysticks detected, using keyboard only\n");
	}
}

void PLAT_quitInput(void) {
	if (gamepad) {
		SDL_GameControllerClose(gamepad);
		gamepad = NULL;
	}
	if (joystick && !gamepad) {
		// Only close if not part of gamepad
		SDL_JoystickClose(joystick);
	}
	joystick = NULL;
	joystick_id = -1;
}

void PLAT_pollInput(void) {
	SDL_Event event;
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	// Process events (window management, hotplug, gamepad events)
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				exit(0);
				break;

			// Hotplug support
			case SDL_CONTROLLERDEVICEADDED:
				if (!gamepad && !joystick) {
					LOG_info("dev platform: Controller connected\n");
					openGamepad(event.cdevice.which);
				}
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				if (gamepad && event.cdevice.which == joystick_id) {
					LOG_info("dev platform: Controller disconnected\n");
					SDL_GameControllerClose(gamepad);
					gamepad = NULL;
					joystick = NULL;
					joystick_id = -1;
				}
				break;

			case SDL_JOYDEVICEADDED:
				if (!gamepad && !joystick) {
					LOG_info("dev platform: Joystick connected\n");
					openGamepad(event.jdevice.which);
				}
				break;

			case SDL_JOYDEVICEREMOVED:
				if (joystick && !gamepad && event.jdevice.which == joystick_id) {
					LOG_info("dev platform: Joystick disconnected\n");
					SDL_JoystickClose(joystick);
					joystick = NULL;
					joystick_id = -1;
				}
				break;
		}
	}

	// Reset transient state
	pad.just_pressed = BTN_NONE;
	pad.just_released = BTN_NONE;
	pad.just_repeated = BTN_NONE;

	// Build current button state
	int now_pressed = 0;

	// Keyboard input (always available)
	if (keys[SDL_SCANCODE_UP]) 		now_pressed |= BTN_UP;
	if (keys[SDL_SCANCODE_DOWN]) 	now_pressed |= BTN_DOWN;
	if (keys[SDL_SCANCODE_LEFT]) 	now_pressed |= BTN_LEFT;
	if (keys[SDL_SCANCODE_RIGHT]) 	now_pressed |= BTN_RIGHT;

	if (keys[SDL_SCANCODE_X]) 		now_pressed |= BTN_A;
	if (keys[SDL_SCANCODE_Z]) 		now_pressed |= BTN_B;
	if (keys[SDL_SCANCODE_S]) 		now_pressed |= BTN_X;
	if (keys[SDL_SCANCODE_A]) 		now_pressed |= BTN_Y;

	if (keys[SDL_SCANCODE_Q]) 		now_pressed |= BTN_L1;
	if (keys[SDL_SCANCODE_W]) 		now_pressed |= BTN_R1;
	if (keys[SDL_SCANCODE_E]) 		now_pressed |= BTN_L2;
	if (keys[SDL_SCANCODE_R]) 		now_pressed |= BTN_R2;
	if (keys[SDL_SCANCODE_T]) 		now_pressed |= BTN_L3;
	if (keys[SDL_SCANCODE_Y]) 		now_pressed |= BTN_R3;

	if (keys[SDL_SCANCODE_RSHIFT]) 	now_pressed |= BTN_SELECT;
	if (keys[SDL_SCANCODE_RETURN]) 	now_pressed |= BTN_START;
	if (keys[SDL_SCANCODE_ESCAPE]) 	now_pressed |= BTN_MENU;
	if (keys[SDL_SCANCODE_P]) 		now_pressed |= BTN_POWER;
	if (keys[SDL_SCANCODE_EQUALS]) 	now_pressed |= BTN_PLUS;
	if (keys[SDL_SCANCODE_MINUS]) 	now_pressed |= BTN_MINUS;

	// Gamepad input (if connected)
	if (gamepad) {
		// D-pad
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
			now_pressed |= BTN_UP;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
			now_pressed |= BTN_DOWN;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
			now_pressed |= BTN_LEFT;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
			now_pressed |= BTN_RIGHT;

		// Face buttons (using Xbox layout: A=bottom, B=right, X=left, Y=top)
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A))
			now_pressed |= BTN_B;  // Xbox A -> MinUI B
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_B))
			now_pressed |= BTN_A;  // Xbox B -> MinUI A
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_X))
			now_pressed |= BTN_Y;  // Xbox X -> MinUI Y
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_Y))
			now_pressed |= BTN_X;  // Xbox Y -> MinUI X

		// Shoulder buttons
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
			now_pressed |= BTN_L1;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
			now_pressed |= BTN_R1;

		// Triggers (analog treated as digital)
		Sint16 l2 = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		Sint16 r2 = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		if (l2 > 16384) now_pressed |= BTN_L2;  // > 50% pressed
		if (r2 > 16384) now_pressed |= BTN_R2;

		// Stick buttons
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK))
			now_pressed |= BTN_L3;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_RIGHTSTICK))
			now_pressed |= BTN_R3;

		// System buttons
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_BACK))
			now_pressed |= BTN_SELECT;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_START))
			now_pressed |= BTN_START;
		if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_GUIDE))
			now_pressed |= BTN_MENU;

		// Analog sticks as D-pad (left stick)
		Sint16 lx = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
		Sint16 ly = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);
		const int deadzone = 16384; // ~50% deadzone

		if (lx < -deadzone) now_pressed |= BTN_LEFT;
		if (lx > deadzone)  now_pressed |= BTN_RIGHT;
		if (ly < -deadzone) now_pressed |= BTN_UP;
		if (ly > deadzone)  now_pressed |= BTN_DOWN;
	}
	// Joystick input (fallback if not a gamepad)
	else if (joystick) {
		int num_buttons = SDL_JoystickNumButtons(joystick);
		int num_axes = SDL_JoystickNumAxes(joystick);

		// Map first 12 buttons to standard layout
		if (num_buttons > 0 && SDL_JoystickGetButton(joystick, 0)) now_pressed |= BTN_B;
		if (num_buttons > 1 && SDL_JoystickGetButton(joystick, 1)) now_pressed |= BTN_A;
		if (num_buttons > 2 && SDL_JoystickGetButton(joystick, 2)) now_pressed |= BTN_Y;
		if (num_buttons > 3 && SDL_JoystickGetButton(joystick, 3)) now_pressed |= BTN_X;
		if (num_buttons > 4 && SDL_JoystickGetButton(joystick, 4)) now_pressed |= BTN_L1;
		if (num_buttons > 5 && SDL_JoystickGetButton(joystick, 5)) now_pressed |= BTN_R1;
		if (num_buttons > 6 && SDL_JoystickGetButton(joystick, 6)) now_pressed |= BTN_L2;
		if (num_buttons > 7 && SDL_JoystickGetButton(joystick, 7)) now_pressed |= BTN_R2;
		if (num_buttons > 8 && SDL_JoystickGetButton(joystick, 8)) now_pressed |= BTN_SELECT;
		if (num_buttons > 9 && SDL_JoystickGetButton(joystick, 9)) now_pressed |= BTN_START;
		if (num_buttons > 10 && SDL_JoystickGetButton(joystick, 10)) now_pressed |= BTN_L3;
		if (num_buttons > 11 && SDL_JoystickGetButton(joystick, 11)) now_pressed |= BTN_R3;

		// First two axes as analog stick
		if (num_axes >= 2) {
			Sint16 ax0 = SDL_JoystickGetAxis(joystick, 0);
			Sint16 ax1 = SDL_JoystickGetAxis(joystick, 1);
			const int deadzone = 16384;

			if (ax0 < -deadzone) now_pressed |= BTN_LEFT;
			if (ax0 > deadzone)  now_pressed |= BTN_RIGHT;
			if (ax1 < -deadzone) now_pressed |= BTN_UP;
			if (ax1 > deadzone)  now_pressed |= BTN_DOWN;
		}

		// D-pad via hat (if available)
		if (SDL_JoystickNumHats(joystick) > 0) {
			Uint8 hat = SDL_JoystickGetHat(joystick, 0);
			if (hat & SDL_HAT_UP)    now_pressed |= BTN_UP;
			if (hat & SDL_HAT_DOWN)  now_pressed |= BTN_DOWN;
			if (hat & SDL_HAT_LEFT)  now_pressed |= BTN_LEFT;
			if (hat & SDL_HAT_RIGHT) now_pressed |= BTN_RIGHT;
		}
	}

	// Calculate just_pressed and just_released
	pad.just_pressed = now_pressed & ~pad.is_pressed;
	pad.just_released = ~now_pressed & pad.is_pressed;

	// Update current state
	pad.is_pressed = now_pressed;
}

int PLAT_shouldWake(void) {
	// For dev platform, we don't have sleep/wake
	return 0;
}

///////////////////////////////
// Video

SDL_Surface* PLAT_initVideo(void) {
	// Load platform configuration
	platform_config_load(NULL, &platform_config);

	// Apply platform config
	window_width = platform_config.profile.screen_width;
	window_height = platform_config.profile.screen_height;

	// Load MinUI configuration if available
#ifdef USE_CONFIG_SYSTEM
	minui_config_t* config = config_load(NULL);
	if (config) {
		CONFIG_set(config);
		if (DEBUG_enabled()) {
			LOG_info("dev platform: Loaded MinUI configuration\n");
		}
	}
#endif

	// Initialize SDL with joystick support
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
		LOG_error("SDL_Init failed: %s\n", SDL_GetError());
		exit(1);
	}

	// Enable joystick event processing
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_GameControllerEventState(SDL_ENABLE);

	// Create window with profile-specific title
	char window_title[256];
	snprintf(window_title, sizeof(window_title), "MinUI Dev - %s (%dx%d)",
		platform_config.profile.name,
		window_width,
		window_height);

	Uint32 window_flags = SDL_WINDOW_SHOWN;
	if (platform_config.window_mode == WINDOW_MODE_FULLSCREEN) {
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	window = SDL_CreateWindow(
		window_title,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_width,
		window_height,
		window_flags
	);

	if (!window) {
		LOG_error("SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	// Create renderer with vsync if enabled
	Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
	if (platform_config.vsync) {
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

	// Determine pixel format and masks based on config
	int bpp = platform_config.profile.bpp;
	int depth = bpp * 8;
	Uint32 rmask, gmask, bmask, amask;
	Uint32 sdl_pixel_format;

	if (bpp == 16) {
		// RGB565 format
		rmask = 0xF800;
		gmask = 0x07E0;
		bmask = 0x001F;
		amask = 0x0000;
		sdl_pixel_format = SDL_PIXELFORMAT_RGB565;
	} else {
		// ARGB8888 format
		rmask = 0x00FF0000;
		gmask = 0x0000FF00;
		bmask = 0x000000FF;
		amask = 0xFF000000;
		sdl_pixel_format = SDL_PIXELFORMAT_ARGB8888;
	}

	// Create screen surface with profile dimensions and pixel format
	screen = SDL_CreateRGBSurface(
		0,
		window_width,
		window_height,
		depth,
		rmask,
		gmask,
		bmask,
		amask
	);

	if (!screen) {
		LOG_error("SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}

	// Create texture for rendering with matching format
	texture = SDL_CreateTexture(
		renderer,
		sdl_pixel_format,
		SDL_TEXTUREACCESS_STREAMING,
		window_width,
		window_height
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

	LOG_info("dev platform: Video initialized - Profile: %s\n", platform_config.active_profile);
	LOG_info("  Display: %dx%d, %d-bit, VSync: %s\n",
		window_width, window_height, bpp,
		platform_config.vsync ? "ON" : "OFF");

	return screen;
}

SDL_Surface* PLAT_resizeVideo(int w, int h, int pitch) {
	// Resize the screen surface dynamically
	// This is called when content needs a different resolution

	if (!screen || (screen->w == w && screen->h == h)) {
		return screen; // Already correct size
	}

	LOG_info("dev platform: Resizing video surface from %dx%d to %dx%d\n",
		screen->w, screen->h, w, h);

	// Destroy old surface and texture
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = NULL;
	}
	if (screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}

	// Determine pixel format
	int bpp = platform_config.profile.bpp;
	int depth = bpp * 8;
	Uint32 rmask, gmask, bmask, amask;
	Uint32 sdl_pixel_format;

	if (bpp == 16) {
		rmask = 0xF800;
		gmask = 0x07E0;
		bmask = 0x001F;
		amask = 0x0000;
		sdl_pixel_format = SDL_PIXELFORMAT_RGB565;
	} else {
		rmask = 0x00FF0000;
		gmask = 0x0000FF00;
		bmask = 0x000000FF;
		amask = 0xFF000000;
		sdl_pixel_format = SDL_PIXELFORMAT_ARGB8888;
	}

	// Create new surface with requested dimensions
	screen = SDL_CreateRGBSurface(0, w, h, depth, rmask, gmask, bmask, amask);
	if (!screen) {
		LOG_error("PLAT_resizeVideo: Failed to create surface: %s\n", SDL_GetError());
		return NULL;
	}

	// Create new texture
	texture = SDL_CreateTexture(renderer, sdl_pixel_format,
		SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!texture) {
		LOG_error("PLAT_resizeVideo: Failed to create texture: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		screen = NULL;
		return NULL;
	}

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
	platform_config.vsync = vsync;
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
