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
//#include "timer0.h"
#include "button-driver.h"
#include "lcd-driver.h"
#include "rtc.h"

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
typedef uint8_t (*func_p)(uint8_t, uint8_t *);

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

// Motor is on PORTD bit 3 and 5
#define MA_1			(1 << PD3)
#define MA_2			(1 << PD5)
#define InitMotor()		(DDRD |= (MA_1 | MA_2))
#define MotorStop()		(PORTD &= ~(MA_1 | MA_2))
#define MotorForward()	(PORTD |= MA_1)
#define MotorBackward()	(PORTD |= MA_2)


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t idleState(uint8_t key, uint8_t *enter);
uint8_t topMenu(uint8_t key, uint8_t *enter);
uint8_t exitMenu(uint8_t key, uint8_t *enter);
uint8_t setupMenu_mode(uint8_t key, uint8_t *enter);
uint8_t doorOpening(uint8_t key, uint8_t *enter);
uint8_t doorClosing(uint8_t key, uint8_t *enter);

enum {
	ST_IDLE = 0,
	ST_TOP_MENU,
	ST_EXIT_MENU,
	ST_SETUP_MENU_MODE,
	ST_DOOR_OPENING,
	ST_DOOR_CLOSING,
	ST_MAX
};
/* must match states above */
func_p states[ST_MAX] = {idleState, 
						topMenu, 
						exitMenu, 
						setupMenu_mode, 
						doorOpening, 
						doorClosing};


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#define Clear_prescaler() (CLKPR = (1<<CLKPCE),CLKPR = 0)

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t nextMenu(uint8_t currentState, uint8_t key)
{
	if (key == KEY_OPEN) {
	}
	else if (key == KEY_CLOSE) {
	}
	return currentState;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t idleState(uint8_t key, uint8_t *enter)
{
	static uint8_t lastSec = 0;
	rtc_time_t currentTime;
	
	if (*enter) {
		LCD_WriteLine(0, 16, "    --:--:--    ");
		LCD_WriteLine(1, 16, "     cooljc     ");
		MotorStop();
		*enter = 0;
	}
	
	RTC_GetTime (&currentTime);
	if (lastSec != currentTime.m_sec) {
		LCD_WriteTime(currentTime);
		lastSec = currentTime.m_sec;
	}
	
	if (key == KEY_MENU) {
		return ST_TOP_MENU;
	}
	else if (key == KEY_OPEN) {
		return ST_DOOR_OPENING;
	}
	else if (key == KEY_CLOSE) {
		return ST_DOOR_CLOSING;
	}
	
	return ST_IDLE;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t topMenu(uint8_t key, uint8_t *enter)
{
	if (*enter) {
		LCD_WriteLine(0, 16, "== Door Setup ==");
		LCD_WriteLine(1, 16, "  Press Menu    ");
		*enter = 0;
	}
	
	if (key == KEY_MENU) {
		return ST_SETUP_MENU_MODE;
	}
	return nextMenu(ST_TOP_MENU, key);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t exitMenu(uint8_t key, uint8_t *enter)
{
	if (*enter) {
		LCD_WriteLine(0, 16, "==    Exit    ==");
		LCD_WriteLine(1, 16, "  Press Menu    ");
		*enter = 0;
	}
	
	if (key == KEY_MENU) {
		return ST_IDLE;
	}
	return nextMenu(ST_EXIT_MENU, key);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t setupMenu_mode(uint8_t key, uint8_t *enter)
{	
	if (*enter) {
		LCD_WriteLine(0, 16, "== Door Mode  ==");
		LCD_WriteLine(1, 16, "  Press Menu    ");
		*enter = 0;
	}
	
	return nextMenu(ST_SETUP_MENU_MODE, key);
}
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t doorOpening(uint8_t key, uint8_t *enter)
{	
	if (*enter) {
		LCD_WriteLine(0, 16, "Door Opening... ");
		LCD_WriteLine(1, 16, "                ");
		MotorStop();
		MotorBackward();
		*enter = 0;
	}
	
	if (key == KEY_OPEN) {
		//return ST_IDLE;
	}
	else if (key == KEY_CLOSE) {
		return ST_IDLE;
		//return ST_DOOR_CLOSING;
	}
	else if (key == KEY_DOOR_OPEN) {
		return ST_IDLE;
	}
	
	return ST_DOOR_OPENING;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t doorClosing(uint8_t key, uint8_t *enter)
{	
	if (*enter) {
		LCD_WriteLine(0, 16, "Door Closing... ");
		LCD_WriteLine(1, 16, "                ");
		MotorStop();
		MotorForward();
		*enter = 0;
	}
	
	if (key == KEY_OPEN) {
		return ST_DOOR_OPENING;
	}
	else if (key == KEY_CLOSE) {
		return ST_IDLE;
	}
	else if (key == KEY_DOOR_CLOSED) {
		return ST_IDLE;
	}

	return ST_DOOR_CLOSING;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void setDefaultTimes (void)
{
	rtc_time_t times;
	// TODO: Read alarm times from EEP
	times.m_hour = 19;
	times.m_min = 6;
	times.m_sec = 0;
	RTC_SetTime(&times);
	//times.m_hour = 6;
	times.m_min = 8;
	RTC_SetOpenTime(&times);
	times.m_hour = 18;
	RTC_SetCloseTime(&times);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
int main (void)
{
	uint8_t state = ST_IDLE;
	uint8_t nextstate = ST_IDLE;
	uint8_t key = KEY_NONE;
	uint8_t enterState = 1;
	uint8_t alarm = RTC_ALARM_NONE;
	func_p pStateFunc = states[state];
	
	
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
	
	while (1)
	{
		/* get a key press or limit switch */
		key = BUTTON_GetKey();
		
		/* override key in favour of alarm */
		alarm = RTC_TestAlarm();
		if (alarm == RTC_ALARM_OPEN) {
			key = KEY_OPEN;
		}
		else if (alarm == RTC_ALARM_CLOSE) {
			key = KEY_CLOSE;
		}
		
		/* execute the current state function */
		nextstate = pStateFunc(key, &enterState);
		
		if (nextstate != state) {
			pStateFunc = states[nextstate];
			state = nextstate;
			enterState = 1;
		}
		
	} /* end of while(1) */
	return 0;
}


/* EOF */
