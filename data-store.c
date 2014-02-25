/*
 * Filename		: data-store.c
 * Author		: Jon Cross
 * Date			: 25/02/2014
 * Description	: Non-volatile config storage.
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <inttypes.h>

#include "data-store.h"

/* ------------------------------------------------------------------ */
/* Address Map ------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* 
 * 0x0000 -> Open Alarm Hour
 * 0x0001 -> Open Alarm Minute
 * 0x0002 -> Close Alarm Hour
 * 0x0003 -> Close Alarm Minute
 */

#define ADDR_ALARM_OPEN_HOUR	0x00
#define ADDR_ALARM_OPEN_MIN		0x01
#define ADDR_ALARM_CLOSE_HOUR	0x02
#define ADDR_ALARM_CLOSE_MIN	0x03
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void DS_GetOpenAlarm(rtc_time_t *alarm)
{
	alarm->m_hour = eeprom_read_byte((uint8_t*)ADDR_ALARM_OPEN_HOUR);
	alarm->m_min  = eeprom_read_byte((uint8_t*)ADDR_ALARM_OPEN_MIN);
	alarm->m_sec  = 0;
	// some sanity test
	if (alarm->m_hour > 23) {
		alarm->m_hour = 6;
	}
	if (alarm->m_hour > 59) {
		alarm->m_hour = 30;
	}
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void DS_GetCloseAlarm(rtc_time_t *alarm)
{
	alarm->m_hour = eeprom_read_byte((uint8_t*)ADDR_ALARM_CLOSE_HOUR);
	alarm->m_min  = eeprom_read_byte((uint8_t*)ADDR_ALARM_CLOSE_MIN);
	alarm->m_sec  = 0;
	// some sanity test
	if (alarm->m_hour > 23) {
		alarm->m_hour = 18;
	}
	if (alarm->m_hour > 59) {
		alarm->m_hour = 30;
	}
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void DS_SetOpenAlarm(rtc_time_t *alarm)
{
	eeprom_write_byte((uint8_t*)ADDR_ALARM_OPEN_HOUR, alarm->m_hour);
	eeprom_write_byte((uint8_t*)ADDR_ALARM_OPEN_MIN, alarm->m_min);
	
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void DS_SetCloseAlarm(rtc_time_t *alarm)
{
	eeprom_write_byte((uint8_t*)ADDR_ALARM_CLOSE_HOUR, alarm->m_hour);
	eeprom_write_byte((uint8_t*)ADDR_ALARM_CLOSE_MIN, alarm->m_min);
}
