/*
 * Filename		: rtc.c
 * Author		: Jon Cross
 * Date			: 24/02/2014
 * Description	: Real Time Clock for coop door.
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
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "common.h"
#include "rtc.h"

volatile rtc_time_t clock;
volatile rtc_time_t alarm_open;
volatile rtc_time_t alarm_close;

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_Init (void)
{
	// Use timer1 16 bit.
	// Divide system clock by 1024 (16MHz/1024 = 15625 = 1s)
	TCCR1A = 0;
	TCCR1B = 0x05;
	
	TCCR1C = 0x80; // force compare A
	
	//OCR1AH = ((15625 << 8) & 0xff);
	//OCR1AL = (15625 & 0xff);
	OCR1A = 15625;
	TIMSK1 = 0x02; //output compare A match interrupt enable
	
	TCNT1 = 0;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_SetTime (rtc_time_t *newTime)
{
	clock.m_sec = newTime->m_sec;
	clock.m_min = newTime->m_min;
	clock.m_hour = newTime->m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_SetOpenTime (rtc_time_t *newTime)
{
	alarm_open.m_sec = newTime->m_sec;
	alarm_open.m_min = newTime->m_min;
	alarm_open.m_hour = newTime->m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_SetCloseTime (rtc_time_t *newTime)
{
	alarm_close.m_sec = newTime->m_sec;
	alarm_close.m_min = newTime->m_min;
	alarm_close.m_hour = newTime->m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_GetTime (rtc_time_t *pTime)
{
	pTime->m_sec = clock.m_sec;
	pTime->m_min = clock.m_min;
	pTime->m_hour = clock.m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_GetOpenTime (rtc_time_t *pTime)
{
	pTime->m_sec = alarm_open.m_sec;
	pTime->m_min = alarm_open.m_min;
	pTime->m_hour = alarm_open.m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_GetCloseTime (rtc_time_t *pTime)
{
	pTime->m_sec = alarm_close.m_sec;
	pTime->m_min = alarm_close.m_min;
	pTime->m_hour = alarm_close.m_hour;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t RTC_TestAlarm (void)
{
	rtc_time_t alarms[2] = {alarm_open, alarm_close};
	uint8_t loop = 0;
	for (loop=0; loop<2; loop++) {
		if ((alarms[loop].m_hour == clock.m_hour) &&
				(alarms[loop].m_min == clock.m_min) && 
				(alarms[loop].m_sec == clock.m_sec)) {
			return loop+RTC_ALARM_OPEN;
		}
	}
	return RTC_ALARM_NONE;
}

#define Led1Toggle()	(PIND |= (1 << PD2))
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
	clock.m_sec++;
	if (clock.m_sec == 60) {
		clock.m_sec = 0;
		clock.m_min++;
		if (clock.m_min == 60) {
			clock.m_min = 0;
			clock.m_hour++;
			if (clock.m_hour == 24) {
				clock.m_hour = 0;
			}
		}
	}
	Led1Toggle();
}

/* EOF */
