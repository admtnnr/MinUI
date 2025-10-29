// dev/platform/platform.h
// Development platform using SDL2 for cross-platform development and testing

#ifndef PLATFORM_H
#define PLATFORM_H

///////////////////////////////

#include "sdl.h"

///////////////////////////////
// Keyboard button mappings (matching common emulator layouts)

// D-pad
#define	BUTTON_UP		SDLK_UP
#define	BUTTON_DOWN		SDLK_DOWN
#define	BUTTON_LEFT		SDLK_LEFT
#define	BUTTON_RIGHT	SDLK_RIGHT

// Face buttons
#define	BUTTON_A		SDLK_x			// X key
#define	BUTTON_B		SDLK_z			// Z key
#define	BUTTON_X		SDLK_s			// S key
#define	BUTTON_Y		SDLK_a			// A key

// Shoulder buttons
#define	BUTTON_L1		SDLK_q
#define	BUTTON_R1		SDLK_w
#define	BUTTON_L2		SDLK_e
#define	BUTTON_R2		SDLK_r
#define BUTTON_L3 		SDLK_t
#define BUTTON_R3 		SDLK_y

// System buttons
#define	BUTTON_SELECT	SDLK_RSHIFT
#define	BUTTON_START	SDLK_RETURN
#define	BUTTON_MENU		SDLK_ESCAPE
#define	BUTTON_POWER	SDLK_p
#define	BUTTON_PLUS		SDLK_EQUALS
#define	BUTTON_MINUS	SDLK_MINUS

///////////////////////////////
// CODE values (for keycodes)

#define CODE_UP			SDLK_UP
#define CODE_DOWN		SDLK_DOWN
#define CODE_LEFT		SDLK_LEFT
#define CODE_RIGHT		SDLK_RIGHT

#define CODE_SELECT		SDLK_RSHIFT
#define CODE_START		SDLK_RETURN

#define CODE_A			SDLK_x
#define CODE_B			SDLK_z
#define CODE_X			SDLK_s
#define CODE_Y			SDLK_a

#define CODE_L1			SDLK_q
#define CODE_R1			SDLK_w
#define CODE_L2			SDLK_e
#define CODE_R2			SDLK_r
#define CODE_L3			SDLK_t
#define CODE_R3			SDLK_y

#define CODE_MENU		SDLK_ESCAPE
#define CODE_POWER		SDLK_p

#define CODE_PLUS		SDLK_EQUALS
#define CODE_MINUS		SDLK_MINUS

///////////////////////////////
// JOY values (for joystick - to be implemented in Phase 3.3)

#define JOY_UP			JOY_NA
#define JOY_DOWN		JOY_NA
#define JOY_LEFT		JOY_NA
#define JOY_RIGHT		JOY_NA

#define JOY_SELECT		JOY_NA
#define JOY_START		JOY_NA

#define JOY_A			JOY_NA
#define JOY_B			JOY_NA
#define JOY_X			JOY_NA
#define JOY_Y			JOY_NA

#define JOY_L1			JOY_NA
#define JOY_R1			JOY_NA
#define JOY_L2			JOY_NA
#define JOY_R2			JOY_NA
#define JOY_L3			JOY_NA
#define JOY_R3			JOY_NA

#define JOY_MENU		JOY_NA
#define JOY_POWER		JOY_NA
#define JOY_PLUS		JOY_NA
#define JOY_MINUS		JOY_NA

///////////////////////////////
// Button shortcuts

#define BTN_RESUME			BTN_A
#define BTN_SLEEP 			BTN_POWER
#define BTN_WAKE 			BTN_POWER
#define BTN_MOD_VOLUME 		BTN_NONE
#define BTN_MOD_BRIGHTNESS 	BTN_MENU
#define BTN_MOD_PLUS 		BTN_PLUS
#define BTN_MOD_MINUS 		BTN_MINUS

///////////////////////////////
// Display defaults (configurable via platform.conf in Phase 3.2)

#define FIXED_SCALE 	2
#define FIXED_WIDTH		640
#define FIXED_HEIGHT	480
#define FIXED_BPP		4		// 32-bit RGBA for SDL2
#define FIXED_DEPTH		(FIXED_BPP * 8)
#define FIXED_PITCH		(FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE		(FIXED_PITCH * FIXED_HEIGHT)

///////////////////////////////
// Platform-specific paths

#define SDCARD_PATH "/tmp/minui_dev"
#define MUTE_VOLUME_RAW 0

///////////////////////////////

#endif
