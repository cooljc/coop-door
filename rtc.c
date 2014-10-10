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
#ifdef DS1307_BOARD
#include "i2cmaster.h"
#endif /* #ifdef DS1307_BOARD */

volatile rtc_time_t clock;
volatile rtc_time_t alarm_open;
volatile rtc_time_t alarm_close;
volatile uint32_t secondTick;

#ifdef DS1307_BOARD
#define RTC_SLAVE_ADDR 0xD0
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static uint8_t rtc_bcd2dec (uint8_t bcd)
{
	uint8_t cnt, ten = 1;
	uint8_t dec = 0;

	for (cnt = 0; cnt < 2 && bcd != 0; cnt++)
	{
		dec += (bcd & 0x0F) * ten;
		bcd >>= 4;
		ten *= 10;
	}
	return (dec);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static uint8_t rtc_dec2bcd (uint8_t dec)
{
	uint8_t bcd, shift;

	bcd = 0;
	shift = 0;

	while (dec > 0 )
	{
		bcd += (dec % 10) << shift;
		dec /= 10;
		shift += 4;
	}
	return bcd;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static void rtc_getTimeFromRTC (rtc_time_t *newTime)
{
	uint8_t data[3];
	i2c_start (RTC_SLAVE_ADDR|I2C_WRITE);
	i2c_write (0x00);
	i2c_rep_start (RTC_SLAVE_ADDR|I2C_READ); /* set READ bit */
	data[0] = i2c_readAck();
	data[1] = i2c_readAck();
	data[2] = i2c_readNak();
	i2c_stop ();
	
	newTime->m_sec  = rtc_bcd2dec( (data[0] & 0x7f) );
	newTime->m_min  = rtc_bcd2dec( (data[1] & 0x7f) );
	newTime->m_hour = rtc_bcd2dec( (data[2] & 0x3f) );
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static void rtc_writeTimeToRTC (rtc_time_t *newTime)
{
	uint8_t data[4];
	data[0] = 0x00; /* sets the first address*/
	data[1] = (rtc_dec2bcd(newTime->m_sec) & 0x7f);
	data[2] = (rtc_dec2bcd(newTime->m_min) & 0x7f);
	data[3] = (rtc_dec2bcd(newTime->m_hour) & 0x3f) | 0x40; /* set 24hr mode */
	
	i2c_start (RTC_SLAVE_ADDR|I2C_WRITE);
	i2c_write (data[0]);
	i2c_write (data[1]);
	i2c_write (data[2]);
	i2c_write (data[3]);
	i2c_stop ();
}
#endif /* DS1307_BOARD */

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
void RTC_SyncTime (void)
{
#ifdef DS1307_BOARD
	rtc_time_t newTime;
	rtc_getTimeFromRTC (&newTime);
	clock.m_sec = newTime.m_sec;
	clock.m_min = newTime.m_min;
	clock.m_hour = newTime.m_hour;
#endif /* #ifdef DS1307_BOARD */
}


/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_SetTime (rtc_time_t *newTime)
{
#ifdef DS1307_BOARD
	rtc_writeTimeToRTC (newTime);
#endif /* #ifdef DS1307_BOARD */
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

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint32_t RTC_GetSecondTick (void)
{
	return secondTick;
}

#ifdef DS1307_BOARD
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_SetDate (rtc_date_t *newDate)
{
	uint8_t data[5];
	
	data[0] = 0x03; /* set register to DAY */
	data[1] = (rtc_dec2bcd(newDate->m_dayNumber) & 0x07);
	data[2] = (rtc_dec2bcd(newDate->m_day) & 0x3f);
	data[3] = (rtc_dec2bcd(newDate->m_month) & 0x1f);
	data[4] = (rtc_dec2bcd(newDate->m_year) & 0xff);
	
	i2c_start (RTC_SLAVE_ADDR|I2C_WRITE);
	i2c_write (data[0]);
	i2c_write (data[1]);
	i2c_write (data[2]);
	i2c_write (data[3]);
	i2c_write (data[4]);
	i2c_stop ();
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void RTC_GetDate (rtc_date_t *date)
{
	uint8_t data[4];
	
	i2c_start (RTC_SLAVE_ADDR|I2C_WRITE);
	i2c_write (0x03); /* set register to DAY */
	i2c_rep_start (RTC_SLAVE_ADDR|I2C_READ); /* set READ bit */
	data[0] = i2c_readAck();
	data[1] = i2c_readAck();
	data[2] = i2c_readAck();
	data[3] = i2c_readNak();
	i2c_stop ();
	
	date->m_dayNumber = rtc_bcd2dec( (data[0] & 0x07) );
	date->m_day       = rtc_bcd2dec( (data[1] & 0x3f) );
	date->m_month     = rtc_bcd2dec( (data[2] & 0x1f) );
	date->m_year      = rtc_bcd2dec( (data[3] & 0xff) );
}
#endif

#define Led1Toggle()	(PIND |= (1 << PD2))
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
	clock.m_sec++;
	secondTick++;
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
