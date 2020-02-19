/*
 * FinalBurn Alpha for Dingux/OpenDingux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <SDL/SDL.h>
#include <unordered_map>

#include "burner.h"
#include "snd.h"
#include "sdl_menu.h"
#include "sdl_input.h"
#include "sdl_run.h"

#define KEYPAD_UP       0x0001
#define KEYPAD_DOWN     0x0002
#define KEYPAD_LEFT     0x0004
#define KEYPAD_RIGHT    0x0008
#define KEYPAD_COIN     0x0010
#define KEYPAD_START    0x0020
#define KEYPAD_FIRE1    0x0040
#define KEYPAD_FIRE2    0x0080
#define KEYPAD_FIRE3    0x0100
#define KEYPAD_FIRE4    0x0200
#define KEYPAD_FIRE5    0x0400
#define KEYPAD_FIRE6    0x0800

#define BUTTON_UP       0x0001
#define BUTTON_DOWN     0x0002
#define BUTTON_LEFT     0x0004
#define BUTTON_RIGHT    0x0008
#define BUTTON_SELECT   0x0010
#define BUTTON_START    0x0020
#define BUTTON_A        0x0040
#define BUTTON_B        0x0080
#define BUTTON_X        0x0100
#define BUTTON_Y        0x0200
#define BUTTON_SL       0x0400
#define BUTTON_SR       0x0800
#define BUTTON_QT       0x1000
#define BUTTON_PAUSE    0x2000
#define BUTTON_QSAVE    0x4000
#define BUTTON_QLOAD    0x8000
#define BUTTON_MENU     0x10000
#define BUTTON_L2       0x20000
#define BUTTON_R2       0x40000
#define BUTTON_L3       0x80000
#define BUTTON_R3       0x100000

char joyCount = 0;
SDL_Joystick *joys[4];

unsigned int FBA_KEYPAD[4];
unsigned char DiagRequest = 0;
unsigned char ServiceRequest = 0;
unsigned char TestRequest = 0;
unsigned char P1P2Start = 0;
std::unordered_map<int, int> button_map;
bool rjoys = false;

// external functions
void ChangeFrameskip(); // run.cpp
static int keystick = 0;
static int keypad = 0; // redefinable keys
static int keypc = 0;  // non-redefinabe keys

// Autofire state:
// 0 key up
// 1 first action or last action is auto up
// 2 last action is auto down
typedef struct
{
	int keymap;
	int keypc;
	int interval;
	uint16_t state;
	uint16_t enable;
} AUTOFIRE_STATE;

AUTOFIRE_STATE autofire_state[6];
int autofire_count;
int power_pressed = 0;
void sdl_input_read(bool process_autofire) // called from do_keypad()
{
	int auto_it = process_autofire ? autofire_count : 0;
	while(1) {
		SDL_Event event;
		memset(&event, 0, sizeof(SDL_Event));
		// process autofire
		if (process_autofire && auto_it > 0) {
			bool handle = false;
			for (int it = auto_it - 1; it >= 0; it--) {
				auto_it--;
				if (autofire_state[it].enable || autofire_state[it].state ) {
					if ((nCurrentFrame % autofire_state[it].interval != 0) && autofire_state[it].enable && (autofire_state[it].state!=2) ) {
						continue;
					}
					event.type = (autofire_state[it].state != 1)? SDL_KEYDOWN : SDL_KEYUP;
					autofire_state[it].state = (autofire_state[it].state==1)? 0 : 1;
					event.key.keysym.sym = (SDLKey)autofire_state[it].keymap;
					handle = true;
					break;
				}
			}
			if (!handle) {
				break;
			}
		}
		else if( !SDL_PollEvent(&event) ) {
			break;
		}

		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
		{
			//process joystick
			static const int joy_commit_range = 3200;
			int axisval = event.jaxis.value;
			int jx = 0, jy = 1;
			if (options.rotate == 2) {//-180 right
				jx = 2;
				jy = 3;
			}
			if (event.jaxis.axis == jx) {// X axis
				keypad &= ~(KEYPAD_LEFT | KEYPAD_RIGHT);
				if (axisval > joy_commit_range)
					keypad |= KEYPAD_RIGHT;
				else if (axisval < -joy_commit_range)
					keypad |= KEYPAD_LEFT;
			} else if (event.jaxis.axis == jy) {// Y axis
				keypad &= ~(KEYPAD_UP | KEYPAD_DOWN);
				if (axisval > joy_commit_range)
					keypad |= KEYPAD_DOWN;
				else if (axisval < -joy_commit_range)
					keypad |= KEYPAD_UP;
			}
			keystick = keypad;
			break;
		}
		case SDL_KEYUP:
			// FBA keypresses
			if (event.key.keysym.sym == keymap.up) keypad &= ~KEYPAD_UP;
			else if (event.key.keysym.sym == keymap.down) keypad &= ~KEYPAD_DOWN;
			else if (event.key.keysym.sym == keymap.left) keypad &= ~KEYPAD_LEFT;
			else if (event.key.keysym.sym == keymap.right) keypad &= ~KEYPAD_RIGHT;
			else if (event.key.keysym.sym == keymap.fire1) keypad &= ~KEYPAD_FIRE1;
			else if (event.key.keysym.sym == keymap.fire2) keypad &= ~KEYPAD_FIRE2;
			else if (event.key.keysym.sym == keymap.fire3) keypad &= ~KEYPAD_FIRE3;
			else if (event.key.keysym.sym == keymap.fire4) keypad &= ~KEYPAD_FIRE4;
			else if (event.key.keysym.sym == keymap.fire5) keypad &= ~KEYPAD_FIRE5;
			else if (event.key.keysym.sym == keymap.fire6) keypad &= ~KEYPAD_FIRE6;
			else if (event.key.keysym.sym == keymap.coin1) keypad &= ~KEYPAD_COIN;
			else if (event.key.keysym.sym == keymap.start1) keypad &= ~KEYPAD_START;

			// handheld keypresses
			if (!process_autofire) {
				if( button_map.find(event.key.keysym.sym) != button_map.end() )
					keypc &= ~button_map[event.key.keysym.sym];
			}
			break;
		case SDL_KEYDOWN:
			// FBA keypresses
			if (event.key.keysym.sym == keymap.up) keypad |= KEYPAD_UP;
			else if (event.key.keysym.sym == keymap.down) keypad |= KEYPAD_DOWN;
			else if (event.key.keysym.sym == keymap.left) keypad |= KEYPAD_LEFT;
			else if (event.key.keysym.sym == keymap.right) keypad |= KEYPAD_RIGHT;
			else if (event.key.keysym.sym == keymap.fire1) keypad |= KEYPAD_FIRE1;
			else if (event.key.keysym.sym == keymap.fire2) keypad |= KEYPAD_FIRE2;
			else if (event.key.keysym.sym == keymap.fire3) keypad |= KEYPAD_FIRE3;
			else if (event.key.keysym.sym == keymap.fire4) keypad |= KEYPAD_FIRE4;
			else if (event.key.keysym.sym == keymap.fire5) keypad |= KEYPAD_FIRE5;
			else if (event.key.keysym.sym == keymap.fire6) keypad |= KEYPAD_FIRE6;
			else if (event.key.keysym.sym == keymap.coin1) keypad |= KEYPAD_COIN;
			else if (event.key.keysym.sym == keymap.start1) keypad |= KEYPAD_START;

			// handheld keypresses
			if (!process_autofire) {
				if( button_map.find(event.key.keysym.sym) != button_map.end() )
					keypc |= button_map[event.key.keysym.sym];
				else if (event.key.keysym.sym == SDLK_HOME) power_pressed = 1;
			}
			break;
		}
		if (process_autofire && auto_it <= 0) {
			break;
		}
	}
}

void do_keypad()
{
	int bVert = !options.rotate && (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL);

	FBA_KEYPAD[0] = 0;
	FBA_KEYPAD[1] = 0;
	FBA_KEYPAD[2] = 0;
	FBA_KEYPAD[3] = 0;
	DiagRequest = 0;
	ServiceRequest = 0;
	TestRequest = 0;
	P1P2Start = 0;

	sdl_input_read(false);
	for (int it = 0; it < autofire_count; it++) {
		if (autofire_state[it].enable == 0 && (keypc & autofire_state[it].keypc)) {
			autofire_state[it].enable = 1;
			autofire_state[it].state = 2; //key down no delay
		}
		if (autofire_state[it].enable != 0 && !(keypc & autofire_state[it].keypc)) {
			autofire_state[it].enable = 0;
		}
	}
	sdl_input_read(true);

	// process redefinable keypresses

	if(options.rotate == 1 ) { //-90
		if (keypad & KEYPAD_UP) FBA_KEYPAD[0] |= bVert ? KEYPAD_LEFT : KEYPAD_UP;
		if (keypad & KEYPAD_DOWN) FBA_KEYPAD[0] |= bVert ? KEYPAD_RIGHT : KEYPAD_DOWN;
		if (keypad & KEYPAD_LEFT) FBA_KEYPAD[0] |= bVert ? KEYPAD_DOWN : KEYPAD_LEFT;
		if (keypad & KEYPAD_RIGHT) FBA_KEYPAD[0] |= bVert ? KEYPAD_UP : KEYPAD_RIGHT;
	}
	else if(options.rotate == 2) {//-180
		if (keypad & KEYPAD_LEFT) FBA_KEYPAD[0] |=  KEYPAD_UP;
		if (keypad & KEYPAD_RIGHT) FBA_KEYPAD[0] |= KEYPAD_DOWN;
		if (keypad & KEYPAD_DOWN) FBA_KEYPAD[0] |=  KEYPAD_LEFT;
		if (keypad & KEYPAD_UP) FBA_KEYPAD[0] |=  KEYPAD_RIGHT;
	}
	else {
		if (keypad & KEYPAD_UP) FBA_KEYPAD[0] |= bVert ? KEYPAD_LEFT : KEYPAD_UP;
		if (keypad & KEYPAD_DOWN) FBA_KEYPAD[0] |= bVert ? KEYPAD_RIGHT : KEYPAD_DOWN;
		if (keypad & KEYPAD_LEFT) FBA_KEYPAD[0] |= bVert ? KEYPAD_DOWN : KEYPAD_LEFT;
		if (keypad & KEYPAD_RIGHT) FBA_KEYPAD[0] |= bVert ? KEYPAD_UP : KEYPAD_RIGHT;
	}

	if (keypad & KEYPAD_COIN) FBA_KEYPAD[0] |= KEYPAD_COIN;
	if (keypad & KEYPAD_START) FBA_KEYPAD[0] |= KEYPAD_START;

	if (keypad & KEYPAD_FIRE1) FBA_KEYPAD[0] |= KEYPAD_FIRE1;		// A
	if (keypad & KEYPAD_FIRE2) FBA_KEYPAD[0] |= KEYPAD_FIRE2;		// B
	if (keypad & KEYPAD_FIRE3) FBA_KEYPAD[0] |= KEYPAD_FIRE3;		// X
	if (keypad & KEYPAD_FIRE4) FBA_KEYPAD[0] |= KEYPAD_FIRE4;		// Y
	if (keypad & KEYPAD_FIRE5) FBA_KEYPAD[0] |= KEYPAD_FIRE5;		// L
	if (keypad & KEYPAD_FIRE6) FBA_KEYPAD[0] |= KEYPAD_FIRE6;		// R

	// process non-redefinable keypresses
	if (keypc & BUTTON_QT) {
		GameLooping = false;
		keypc = keypad = 0;
	}

	if (keypc & BUTTON_MENU) {
		keypc = keypad = 0;
		SndPause(1);
		gui_Run();
		SndPause(0);
	}

	if (power_pressed) {
		power_pressed = 0;
		SndPause(1);
		gui_Run();
		SndPause(0);
	}

	if (keypc & BUTTON_PAUSE) {
		bRunPause = !bRunPause;
		SndPause(bRunPause);
		keypc &= ~BUTTON_PAUSE;
	}

	if(!bRunPause) { // savestate fails inside pause
		if (keypc & BUTTON_QSAVE) {
			StatedSave(nSavestateSlot);
			keypc &= ~BUTTON_QSAVE;
		}

		if (keypc & BUTTON_QLOAD) {
			StatedLoad(nSavestateSlot);
			keypc &= ~BUTTON_QLOAD;
			bRunPause = 0;
		}
	}
	if ((keypc & BUTTON_SL) && (keypc & BUTTON_SR)) {
		if (keypc & BUTTON_Y) {
			ChangeFrameskip();
			keypc &= ~BUTTON_Y;
		} else if (keypc & BUTTON_B && !bRunPause) {
			StatedSave(nSavestateSlot);
			extern void save_state_preview(bool,bool);
			save_state_preview(true,false);
			keypc &= ~BUTTON_B;
		} else if (keypc & BUTTON_A && !bRunPause) {
			StatedLoad(nSavestateSlot);
			keypc &= ~BUTTON_A;
			bRunPause = 0;
		} else if (keypc & BUTTON_START) {
			keypc = keypad = 0;
			SndPause(1);
			gui_Run();
			SndPause(0);
		} else if (keypc & BUTTON_SELECT) DiagRequest = 1;
	}
	else if ((keypc & BUTTON_START) && (keypc & BUTTON_SELECT)) P1P2Start = 1;
	else if (keypc & BUTTON_L2) ServiceRequest = 1;
	else if (keypc & BUTTON_R2) TestRequest = 1;
}

void sdl_input_init()
{
	rjoys = false;
	power_pressed = 0;
	joyCount = SDL_NumJoysticks();
	if (joyCount > 5) joyCount = 5;
	printf("%d Joystick(s) Found\n", joyCount);

	if (joyCount > 0) {
		for (int i = 0; i < joyCount; i++) {
			printf("%s\t",SDL_JoystickName(i));
			joys[i] = SDL_JoystickOpen(i);
			printf("Hats %d\t",SDL_JoystickNumHats(joys[i]));
			printf("Buttons %d\t",SDL_JoystickNumButtons(joys[i]));
			printf("Axis %d\n",SDL_JoystickNumAxes(joys[i]));
			if(!rjoys)
				rjoys = (SDL_JoystickNumAxes(joys[i])>2);
		}
	}
	button_map_init();
	sdl_autofire_init();
}

void button_map_init() {
	button_map.clear();
	button_map[SDLK_LCTRL] = BUTTON_A;
	button_map[SDLK_LALT] = BUTTON_B;
	button_map[SDLK_SPACE] = BUTTON_X;
	button_map[SDLK_LSHIFT] = BUTTON_Y;
	button_map[SDLK_TAB] = BUTTON_SL;
	button_map[SDLK_BACKSPACE] = BUTTON_SR;
	button_map[SDLK_ESCAPE] = BUTTON_SELECT;
	button_map[SDLK_RETURN] = BUTTON_START;
	button_map[SDLK_PAGEUP] = BUTTON_L2;
	button_map[SDLK_PAGEDOWN] = BUTTON_R2;
	button_map[SDLK_KP_DIVIDE] = BUTTON_L3;
	button_map[SDLK_KP_PERIOD] = BUTTON_R3;
	button_map[SDLK_q] = BUTTON_QT;
	button_map[SDLK_p] = BUTTON_PAUSE;
	button_map[SDLK_s] = BUTTON_QSAVE;
	button_map[SDLK_l] = BUTTON_QLOAD;
	button_map[SDLK_m] = BUTTON_MENU;
}

void sdl_autofire_init() {
	autofire_count = 0;
	CFG_AUTOFIRE_KEY *af = &autofire.fire1;
	int *key = &keymap.fire1;
	for(int i = 0; i < 6; af++, key++, i++) {
		if (af->fps != 0) {
			autofire_state[autofire_count].keymap = *key;
			autofire_state[autofire_count].interval = af->fps / 2;
			autofire_state[autofire_count].state = 0;
			autofire_state[autofire_count].enable = 0;

			int keydef = 0;
			if( button_map.find(af->key) != button_map.end() )
				keydef = button_map[af->key];

			if (key != 0) {
				autofire_state[autofire_count++].keypc = keydef;
			}
		}
	}
}
