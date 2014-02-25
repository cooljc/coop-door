/*
 * Filename		: button-driver.c
 * Author		: Jon Cross
 * Date			: 23/02/2014
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
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <inttypes.h>

#include "common.h"
#include "button-driver.h"


volatile char KEY = KEY_NONE;
volatile bool KEY_VALID = FALSE;

/* ------------------------------------------------------------------ *
 * 
 * Buttons:-
 * Open        : PORTC1 (PCINT9)
 * Close       : PORTC3 (PCINT11)
 * Menu        : PORTC4 (PCINT12)
 * Door Open   : PORTB0 [Di8]
 * Door Closed : PORTD7 [Di7]
 * ------------------------------------------------------------------ */
#define PIND_MASK	(1<<PIND7)
#define PINC_MASK	((1<<PINC1) | (1<<PINC3) | (1<<PINC4))
#define PINB_MASK	(1<<PINB0)

//char CountdownTimerHandle;

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void button_change_interrupt(void)
{
	uint8_t	key = KEY_NONE;
	
	uint8_t buttons = ~PINC;
	uint8_t portb = ~PINB;
	uint8_t portd = ~PIND;
	
	/* check buttons */
	if (buttons & (1<<PINC1)) {
		key = KEY_OPEN;
	}
	else if (buttons & (1<<PINC3)) {
		key = KEY_CLOSE;
	}
	else if (buttons & (1<<PINC4)) {
		//key = KEY_MENU;
	}
	
	/* TODO: Handle debounce!! */
	_delay_ms(100);
	if (key != KEY_NONE) {
		if (!KEY_VALID) {
			/* Store key in global key buffer */
			KEY = key;
			KEY_VALID = TRUE;
		}
	}
	
	/* Delete pin change interrupt flags */
	PCIFR = (1<<PCIF1) | (1<<PCIF0);
}


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
ISR(PCINT0_vect)
{
    button_change_interrupt();
}
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
ISR(PCINT1_vect)
{
    button_change_interrupt();
}


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void BUTTON_Init(void)
{
	/* setup port directions */
	DDRB &= ~(PINB_MASK);
	PORTB |= PINB_MASK; /* enable pullup */
	DDRC &= ~(PINC_MASK);
	DDRD &= ~(PIND_MASK);
	
	/* setup interrupts */
	//PCMSK0 |= (1<<PCINT0);
	PCMSK1 |= ((1<<PCINT9) | (1<<PCINT11) | (1<<PCINT12));
	
	PCIFR = (1<<PCIF1) | (1<<PCIF0);
    PCICR = (1<<PCIE1) | (1<<PCIE1);
    
    //CountdownTimerHandle = Timer0_AllocateCountdownTimer();
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t BUTTON_GetKey(void)
{
	uint8_t k = KEY_NONE;

    cli(); 

	/* check for unread key in buffer */
    if (KEY_VALID) {
        k = KEY;
        KEY_VALID = FALSE;
    }

    sei(); 

	return k;
}

/* EOF */
