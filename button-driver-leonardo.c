/*
 * Filename		: button-driver-leonardo.c
 * Author		: Jon Cross
 * Date			: 05/10/2014
 * Description	: Driver for the coop door controller keys..
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
 */


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <inttypes.h>

#include "common.h"
#include "button-driver.h"


/* ------------------------------------------------------------------ *
 *
 * Buttons:-
 * Open        : PORTF7 (A0)
 * Menu        : PORTF6 (A1)
 * Close       : PORTF5 (A2)
 * Door Open   : PORTF1 (A4)
 * Door Closed : PORTF0 (A5)
 * ------------------------------------------------------------------ */
#define PINF_MASK	((1<<PINF0) | (1<<PINF1) | (1<<PINF5) | (1<<PINF6) | (1<<PINF7))

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t button_read_keys (void)
{
	uint8_t	key = KEY_NONE;
	uint8_t buttons = ~PINF;

	/* check buttons */
	if (buttons & (1<<PINF7)) {
		key |= KEY_OPEN;
	}
	else if (buttons & (1<<PINF5)) {
		key |= KEY_CLOSE;
	}
	else if (buttons & (1<<PINF6)) {
		key |= KEY_MENU;
	}
	else if (buttons & (1<<PINF1)) {
		key |= KEY_DOOR_OPEN;
	}
	else if (buttons & (1<<PINF0)) {
		key |= KEY_DOOR_CLOSED;
	}
	return key;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void BUTTON_Init(void)
{
	/* setup port directions */
	DDRF &= ~(PINF_MASK);
	PORTF |= PINF_MASK; /* enable pullup */
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t BUTTON_GetKey(void)
{
	uint8_t k = KEY_NONE;
	uint8_t	key1 = KEY_NONE;
	uint8_t key2 = KEY_NONE;

	/* read key */
	key1 = button_read_keys();

	/* Handle debounce!! */
	_delay_ms(50);

	/* read key again */
	key2 = button_read_keys();

	if ((key1 == key2) && (key1 != KEY_NONE)) {
		k = key1;
	}

	return k;
}

/* EOF */
