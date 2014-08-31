/*
 * Filename		: coop-door.c
 * Author		: Jon Cross
 * Date			: 23/02/2014
 * Description	: Main entry point and control loop coop door controller.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA  02110-1301  USA.
 *
 */

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#include "wdt_drv.h"
#include "common.h"
#include "button-driver.h"
#include "lcd-driver.h"
#include "rtc.h"
#include "data-store.h"

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */


// LEDs on POP-168 board: PD2 (Di2) & PD4 (Di4) - Tided high
// Switches on POP-168 board: PD2 (Di2) & PD4 (Di4)
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

#define LED1      		(1 << PD2)
#define LED2			(1 << PD4)
#define InitLED()		(DDRD |= (LED1 | LED2))
#define Led1On()		(PORTD |= LED1)
#define Led1Off()		(PORTD &= ~LED1)
#define Led1Toggle()	(PIND |= LED1)
#define Led2On()		(PORTD |= LED2)
#define Led2Off()		(PORTD &= ~LED2)
#define Led2Toggle()	(PIND |= LED2)

#define USE_MOTOR_CHANNEL_B 1

// Motor is on PORTD bit 3 and 5
#ifndef USE_MOTOR_CHANNEL_B
#define MA_1			(1 << PD3)
#define MA_2			(1 << PD5)
#define InitMotor()		(DDRD |= (MA_1 | MA_2))
#define MotorStop()		(PORTD &= ~(MA_1 | MA_2))
#define MotorBrake()	(PORTD |= (MA_1 | MA_2))
#define MotorForward()	(PORTD |= MA_1)
#define MotorBackward()	(PORTD |= MA_2)
#else // USE_MOTOR_CHANNEL_B
#define MB_1			(1 << PB1)
#define MB_2			(1 << PD6)
#define InitMotor()		DDRD |= MB_2; DDRB |= MB_1
#define MotorStop()		PORTD &= ~(MB_2); PORTB &= ~(MB_1)
#define MotorBrake()	PORTD |= MB_2; PORTB |= MB_1
#define MotorForward()	(PORTB |= MB_1)
#define MotorBackward()	(PORTD |= MB_2)
#endif

typedef struct {
	uint8_t 	m_enter;
	uint8_t 	m_key;
	uint8_t 	m_top_menu_state;
	uint8_t 	m_sub_menu_state;
	uint8_t 	m_in_sub_menu;
	uint8_t 	m_setup_change_state;
	uint8_t		m_door_mode;
	uint8_t		m_door_state;
	uint8_t		m_menu_timeout;
	uint8_t		m_temp;
	uint8_t		m_open_sw_inhibit;
	rtc_time_t 	m_time;
} state_params_t;

enum {
	DOOR_STATE_UNKNOWN = 0,
	DOOR_STATE_CLOSED,
	DOOR_STATE_CLOSING,
	DOOR_STATE_OPEN,
	DOOR_STATE_OPENING,
	DOOR_STATE_ERROR,
};

typedef uint8_t (*func_p)(state_params_t *);
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t idleState(state_params_t *params);
uint8_t idleStateError(state_params_t *params);
uint8_t setupMenu(state_params_t *params);
uint8_t exitMenu(state_params_t *params);
uint8_t setupMenu_mode(state_params_t *params);
uint8_t setupMenu_clock(state_params_t *params);
uint8_t setupMenu_open_al(state_params_t *params);
uint8_t setupMenu_close_al(state_params_t *params);
uint8_t doorOpening(state_params_t *params);
uint8_t doorClosing(state_params_t *params);

enum {
	ST_IDLE = 0,
	ST_IDLE_ERROR,
	ST_SETUP_MENU,
	ST_EXIT_MENU,
	ST_SETUP_MENU_MODE,
	ST_SETUP_MENU_CLOCK,
	ST_SETUP_MENU_OPEN_AL,
	ST_SETUP_MENU_CLOSE_AL,
	ST_DOOR_OPENING,
	ST_DOOR_CLOSING,
	ST_MAX
};

/* must match states above */
func_p states[ST_MAX] = {idleState,
						idleStateError,
						setupMenu,
						exitMenu,
						setupMenu_mode,
						setupMenu_clock,
						setupMenu_open_al,
						setupMenu_close_al,
						doorOpening,
						doorClosing};

#define TOP_MENU_STATE_MAX 2
uint8_t top_menu_states[TOP_MENU_STATE_MAX] = {ST_SETUP_MENU, ST_EXIT_MENU};

#define SUB_MENU_STATE_MAX 5
uint8_t sub_menu_states[SUB_MENU_STATE_MAX] = {ST_SETUP_MENU_MODE,
											   ST_SETUP_MENU_CLOCK,
											   ST_SETUP_MENU_OPEN_AL,
											   ST_SETUP_MENU_CLOSE_AL,
											   ST_EXIT_MENU};

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#define Clear_prescaler() (CLKPR = (1<<CLKPCE),CLKPR = 0)

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t nextTopMenu(uint8_t currentState, state_params_t *params)
{
	uint8_t retState = currentState;
	if (params->m_key == KEY_OPEN) {
		params->m_top_menu_state++;
		if (params->m_top_menu_state == TOP_MENU_STATE_MAX) {
			params->m_top_menu_state = 0;
		}
		retState = top_menu_states[params->m_top_menu_state];
	}
	else if (params->m_key == KEY_CLOSE) {
		params->m_top_menu_state--;
		if (params->m_top_menu_state == 255) {
			params->m_top_menu_state = TOP_MENU_STATE_MAX-1;
		}
		retState = top_menu_states[params->m_top_menu_state];
	}
	return retState;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t nextSubMenu(uint8_t currentState, state_params_t *params)
{
	uint8_t retState = currentState;
	if (params->m_key == KEY_OPEN) {
		params->m_sub_menu_state++;
		if (params->m_sub_menu_state == SUB_MENU_STATE_MAX) {
			params->m_sub_menu_state = 0;
		}
		retState = sub_menu_states[params->m_sub_menu_state];
	}
	else if (params->m_key == KEY_CLOSE) {
		params->m_sub_menu_state--;
		if (params->m_sub_menu_state == 255) {
			params->m_sub_menu_state = SUB_MENU_STATE_MAX-1;
		}
		retState = sub_menu_states[params->m_sub_menu_state];
	}
	return retState;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t SetTimeValue(uint8_t currentState, state_params_t *params)
{
	if (params->m_enter) {
		if (currentState == ST_SETUP_MENU_CLOCK) {
			RTC_GetTime(&params->m_time);
		}
		else if (currentState == ST_SETUP_MENU_OPEN_AL) {
			DS_GetOpenAlarm(&params->m_time);
		}
		else if (currentState == ST_SETUP_MENU_CLOSE_AL) {
			DS_GetCloseAlarm(&params->m_time);
		}
		LCD_WriteLine(0, 16, "                ");
		LCD_WriteLine(1, 16, "                ");
		LCD_WriteTime(params->m_time);
		LCD_SetCursor(LCD_CURSOR_HOUR);
		params->m_enter = 0;
	}

	if (params->m_key == KEY_OPEN) {
		if (params->m_setup_change_state == 1) {
			// update hour
			params->m_time.m_hour++;
			if (params->m_time.m_hour == 24) {
				params->m_time.m_hour = 0;
			}
			LCD_WriteTime(params->m_time);
			LCD_SetCursor(LCD_CURSOR_HOUR);
		}
		else if (params->m_setup_change_state == 2) {
			// update minute
			params->m_time.m_min++;
			if (params->m_time.m_min == 60) {
				params->m_time.m_min = 0;
			}
			LCD_WriteTime(params->m_time);
			LCD_SetCursor(LCD_CURSOR_MIN);
		}
	}
	else if (params->m_key == KEY_CLOSE) {
		if (params->m_setup_change_state == 1) {
			// update hour
			params->m_time.m_hour--;
			if (params->m_time.m_hour == 255) {
				params->m_time.m_hour = 23;
			}
			LCD_WriteTime(params->m_time);
			LCD_SetCursor(LCD_CURSOR_HOUR);
		}
		else if (params->m_setup_change_state == 2) {
			// update minute
			params->m_time.m_min--;
			if (params->m_time.m_min == 255) {
				params->m_time.m_min = 59;
			}
			LCD_WriteTime(params->m_time);
			LCD_SetCursor(LCD_CURSOR_MIN);
		}
	}
	else if (params->m_key == KEY_MENU) {
		params->m_setup_change_state++;
		if (params->m_setup_change_state == 2) {
			// change LCD to min
			LCD_SetCursor(LCD_CURSOR_MIN);
		}
		else if (params->m_setup_change_state == 3) {
			// save time
			LCD_SetCursor(LCD_CURSOR_OFF);
			LCD_WriteLine(0, 16, "Saving...       ");
			if (currentState == ST_SETUP_MENU_CLOCK) {
				RTC_SetTime(&params->m_time);
			}
			else if (currentState == ST_SETUP_MENU_OPEN_AL) {
				DS_SetOpenAlarm(&params->m_time);
				RTC_SetOpenTime(&params->m_time);
			}
			else if (currentState == ST_SETUP_MENU_CLOSE_AL) {
				DS_SetCloseAlarm(&params->m_time);
				RTC_SetCloseTime(&params->m_time);
			}
			params->m_setup_change_state = 4;
		}
	}
	return currentState;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t SetModeValue(uint8_t currentState, state_params_t *params)
{
	if (params->m_enter) {
		LCD_WriteLine(0, 16, "=   Door Mode  =");
		if (params->m_temp == DOOR_MODE_OPEN_CLOSE) {
			LCD_WriteLine(1, 16, "   Open/Close   ");
		}
		else if (params->m_temp == DOOR_MODE_OPEN_ONLY) {
			LCD_WriteLine(1, 16, "   Open Only    ");
		}
		params->m_enter = 0;
	}

	if ((params->m_key == KEY_OPEN) || (params->m_key == KEY_CLOSE)) {
		if (params->m_temp == DOOR_MODE_OPEN_CLOSE) {
			params->m_temp = DOOR_MODE_OPEN_ONLY;
		}
		else {
			params->m_temp = DOOR_MODE_OPEN_CLOSE;
		}
		params->m_enter = 1;
	}
	else if (params->m_key == KEY_MENU) {
		params->m_setup_change_state = 2;
		params->m_door_mode = params->m_temp;
		LCD_WriteLine(0, 16, "Saving...       ");
		LCD_WriteLine(1, 16, "                ");
	}

	return currentState;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t idleState(state_params_t *params)
{
	static uint8_t lastUpdate = 60;
	rtc_time_t currentTime;

	if (params->m_enter) {
#ifdef CLOCK_SHOW_SECONDS
		LCD_WriteLine(0, 16, "    --:--:--    ");
#else
		LCD_WriteLine(0, 16, "     --:--      ");
#endif
		LCD_WriteLine(1, 16, "     cooljc     ");
		MotorStop();
		params->m_enter = 0;
		params->m_in_sub_menu = 0;
		lastUpdate = 60;
	}


#ifdef CLOCK_SHOW_SECONDS
	if (lastUpdate != RTC_GetSecondTick()) {
		RTC_GetTime (&currentTime);
		LCD_WriteTime(currentTime);
		lastUpdate = RTC_GetSecondTick();
	}
#else
	RTC_GetTime (&currentTime);
	if (lastUpdate != currentTime.m_min) {
		LCD_WriteTime(currentTime);
		lastUpdate = currentTime.m_min;
	}
#endif

	if (params->m_key == KEY_MENU) {
		return ST_SETUP_MENU;
	}
	else if (params->m_key == KEY_OPEN) {
		return ST_DOOR_OPENING;
	}
	else if (params->m_key == KEY_CLOSE) {
		return ST_DOOR_CLOSING;
	}

	return ST_IDLE;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t idleStateError(state_params_t *params)
{
	if (params->m_enter) {
		MotorStop();
		params->m_enter = 0;
		params->m_in_sub_menu = 0;
		// close error
		LCD_WriteLine(0, 16, "Door Close Error");
		LCD_WriteLine(1, 16, "Check for dirt! ");
	}
	if (params->m_key == KEY_OPEN) {
		return ST_DOOR_OPENING;
	}
	return ST_IDLE_ERROR;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu(state_params_t *params)
{
	if (params->m_enter) {
		LCD_WriteLine(0, 16, "==    Setup   ==");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
		params->m_in_sub_menu = 0;
	}

	if (params->m_key == KEY_MENU) {
		return ST_SETUP_MENU_MODE;
	}
	return nextTopMenu(ST_SETUP_MENU, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t exitMenu(state_params_t *params)
{
	if (params->m_enter) {
		LCD_WriteLine(0, 16, "==    Exit    ==");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
	}

	if (params->m_in_sub_menu == 1) {
		if (params->m_key == KEY_MENU) {
			params->m_in_sub_menu = 0;
			return ST_SETUP_MENU;
		}
		return nextSubMenu(ST_EXIT_MENU, params);
	}

	if (params->m_key == KEY_MENU) {
		return ST_IDLE;
	}
	return nextTopMenu(ST_EXIT_MENU, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu_mode(state_params_t *params)
{
	if ((params->m_enter) && (params->m_setup_change_state == 0)) {
		LCD_WriteLine(0, 16, "== Door Mode  ==");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
		params->m_in_sub_menu = 1;
	}

	if (params->m_setup_change_state == 1) {
		return SetModeValue(ST_SETUP_MENU_MODE, params);
	}
	else if (params->m_setup_change_state == 2) {
		params->m_enter = 1;
		params->m_setup_change_state = 0;
		DS_SetAlarmMode(params->m_door_mode);
		return ST_SETUP_MENU_MODE;
	}

	if (params->m_key == KEY_MENU) {
		params->m_setup_change_state = 1;
		params->m_enter = 1;
		params->m_temp = params->m_door_mode;
	}

	return nextSubMenu(ST_SETUP_MENU_MODE, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu_clock(state_params_t *params)
{
	if ((params->m_enter) && (params->m_setup_change_state == 0)) {
		LCD_WriteLine(0, 16, "== Set Clock  ==");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
		params->m_in_sub_menu = 1;
	}

	if ((params->m_setup_change_state > 0) && (params->m_setup_change_state < 4)) {
		return SetTimeValue(ST_SETUP_MENU_CLOCK, params);
	}
	else if (params->m_setup_change_state == 4) {
		params->m_enter = 1;
		params->m_setup_change_state = 0;
	}

	if (params->m_key == KEY_MENU) {
		params->m_enter = 1;
		params->m_setup_change_state = 1;
		return ST_SETUP_MENU_CLOCK;
	}

	return nextSubMenu(ST_SETUP_MENU_CLOCK, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu_open_al(state_params_t *params)
{
	if ((params->m_enter) && (params->m_setup_change_state == 0)) {
		LCD_WriteLine(0, 16, "=  Set Open AL =");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
		params->m_in_sub_menu = 1;
	}

	if ((params->m_setup_change_state > 0) && (params->m_setup_change_state < 4)) {
		return SetTimeValue(ST_SETUP_MENU_OPEN_AL, params);
	}
	else if (params->m_setup_change_state == 4) {
		params->m_enter = 1;
		params->m_setup_change_state = 0;
	}

	if (params->m_key == KEY_MENU) {
		params->m_enter = 1;
		params->m_setup_change_state = 1;
		return ST_SETUP_MENU_OPEN_AL;
	}

	return nextSubMenu(ST_SETUP_MENU_OPEN_AL, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu_close_al(state_params_t *params)
{
	if ((params->m_enter) && (params->m_setup_change_state == 0)) {
		LCD_WriteLine(0, 16, "= Set Close AL =");
		LCD_WriteLine(1, 16, "   Press Menu   ");
		params->m_enter = 0;
		params->m_in_sub_menu = 1;
	}

	if ((params->m_setup_change_state > 0) && (params->m_setup_change_state < 4)) {
		return SetTimeValue(ST_SETUP_MENU_CLOSE_AL, params);
	}
	else if (params->m_setup_change_state == 4) {
		params->m_enter = 1;
		params->m_setup_change_state = 0;
	}

	if (params->m_key == KEY_MENU) {
		params->m_enter = 1;
		params->m_setup_change_state = 1;
		return ST_SETUP_MENU_CLOSE_AL;
	}

	return nextSubMenu(ST_SETUP_MENU_CLOSE_AL, params);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t doorOpening(state_params_t *params)
{
	// check if door is already open. If so return to idle state.
	if (params->m_door_state == DOOR_STATE_OPEN) {
		return ST_IDLE;
	}

	if (params->m_enter) {
		LCD_WriteLine(0, 16, "Door Opening... ");
		LCD_WriteLine(1, 16, "                ");
		MotorStop();
		MotorBackward();
		params->m_enter = 0;
		if (params->m_door_state == DOOR_STATE_ERROR) {
			// door is in error state so it needs to be opened to put the
			// spool in the correct winding. This means we need to inhibit
			// the door open switch for a short time to allow it to open.
			params->m_open_sw_inhibit = RTC_GetSecondTick() + 2;
		}
		else {
			params->m_open_sw_inhibit = 0;
		}
		params->m_door_state = DOOR_STATE_OPENING;
	}

	if (params->m_open_sw_inhibit != 0 && params->m_open_sw_inhibit > RTC_GetSecondTick()) {
		return ST_DOOR_OPENING;
	}
	else {
		params->m_open_sw_inhibit = 0 ;
	}

	if ((params->m_key == KEY_DOOR_OPEN) || (params->m_key == KEY_MENU)){
		MotorBrake();
		if (params->m_key == KEY_DOOR_OPEN) {
			// only set state to open if open limit switch is hit
			params->m_door_state = DOOR_STATE_OPEN;
		}
		else {
			// door stopped during open cycle. Now we are in unknown state
			params->m_door_state = DOOR_STATE_UNKNOWN;
		}
		return ST_IDLE;
	}

	return ST_DOOR_OPENING;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t doorClosing(state_params_t *params)
{
	// check if door is already closed. If so return to idle state.
	if (params->m_door_state == DOOR_STATE_CLOSED || params->m_door_state == DOOR_STATE_ERROR) {
		return ST_IDLE;
	}
	if (params->m_enter) {
		LCD_WriteLine(0, 16, "Door Closing... ");
		LCD_WriteLine(1, 16, "                ");
		MotorStop();
		MotorForward();
		params->m_enter = 0;
		params->m_door_state = DOOR_STATE_CLOSING;
		// we want to inhibit open switch for 5 seconds.
		params->m_open_sw_inhibit = RTC_GetSecondTick() + 5;
	}

	if ((params->m_key == KEY_DOOR_CLOSED) || (params->m_key == KEY_MENU)) {
		MotorBrake();
		if (params->m_key == KEY_DOOR_CLOSED) {
			// only set state to closed if door closed limit swith is hit
			params->m_door_state = DOOR_STATE_CLOSED;
		}
		else {
			// has been stopped by menu key so it might be between states
			params->m_door_state = DOOR_STATE_UNKNOWN;
		}
		return ST_IDLE;
	}
	else if (params->m_key == KEY_DOOR_OPEN && params->m_open_sw_inhibit <= RTC_GetSecondTick()) {
		// this is a special case where the bottom of the door is blocked by dirt and the
		// motor has fully unwound and starts opening the door again. We need to stop the
		// motor when it gets to the open switch to stop it buring out.
		MotorBrake();
		// set Door state to Error!!
		params->m_door_state = DOOR_STATE_ERROR;
		return ST_IDLE_ERROR;
	}

	return ST_DOOR_CLOSING;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void setDefaultTimes (void)
{
	rtc_time_t times;

	times.m_hour = 16;
	times.m_min = 0;
	times.m_sec = 0;
	RTC_SetTime(&times);

	// Read alarm times from EEP
	DS_GetOpenAlarm(&times);
	RTC_SetOpenTime(&times);

	DS_GetCloseAlarm(&times);
	RTC_SetCloseTime(&times);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
int main (void)
{
	uint8_t state = ST_IDLE;
	uint8_t nextstate = ST_IDLE;
	uint8_t alarm = RTC_ALARM_NONE;
	uint8_t key = KEY_NONE;
	uint8_t lastKey = KEY_NONE;
	func_p pStateFunc = states[state];
	state_params_t params;

	/* disable watchdog */
	wdt_reset();
	Wdt_clear_flag();
	Wdt_change_enable();
	Wdt_stop();

	Clear_prescaler();
	InitLED();
	Led1On();

	InitMotor();
	MotorStop();

	BUTTON_Init();
	LCD_Init();
	setDefaultTimes();
	RTC_Init();

	sei();

	/* set params initial state */
	params.m_enter = 1;
	params.m_key = KEY_NONE;
	params.m_top_menu_state = 0;
	params.m_sub_menu_state = 0;
	params.m_in_sub_menu = 0;
	params.m_setup_change_state = 0;
	params.m_door_state = DOOR_STATE_UNKNOWN;
	DS_GetAlarmMode(&params.m_door_mode);

	while (1)
	{
		/* get a key press or limit switch */
		key = BUTTON_GetKey();
		if (key != lastKey) {
			params.m_key = key;
			/* only allow open/menu/close to be included in last key */
			if (key == KEY_OPEN || key == KEY_MENU || key == KEY_CLOSE || key == KEY_NONE) {
				lastKey = key;
			}
		}
		else {
			/* prevents key from triggering if it is held down */
			params.m_key = KEY_NONE;
		}

		/* override key in favour of alarm */
		alarm = RTC_TestAlarm();
		if (alarm == RTC_ALARM_OPEN) {
			params.m_key = KEY_OPEN;
		}
		else if ((alarm == RTC_ALARM_CLOSE) && (params.m_door_mode == DOOR_MODE_OPEN_CLOSE)) {
			params.m_key = KEY_CLOSE;
		}

		/* execute the current state function */
		nextstate = pStateFunc(&params);

		if (nextstate != state) {
			pStateFunc = states[nextstate];
			state = nextstate;
			params.m_enter = 1;
			// set timeout for menus
			params.m_menu_timeout = RTC_GetSecondTick() + 20;
		}
		else {
			// no change in state..
			// check if we are in a menu with no activity.
			if (state >= ST_SETUP_MENU && state <= ST_SETUP_MENU_CLOSE_AL) {
				if (params.m_menu_timeout <= RTC_GetSecondTick()) {
					pStateFunc = states[ST_IDLE];
					state = ST_IDLE;
					params.m_enter = 1;
				}
			}
		}

	} /* end of while(1) */
	return 0;
}


/* EOF */
