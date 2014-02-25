/*
 * Filename		: rtc.h
 * Author		: Jon Cross
 * Date			: 23/02/2014
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
#ifndef _RTC_H
#define _RTC_H

typedef struct {
	uint8_t	m_hour;
	uint8_t m_min;
	uint8_t m_sec;
} rtc_time_t;

enum {
	RTC_ALARM_NONE = 0,
	RTC_ALARM_OPEN,
	RTC_ALARM_CLOSE
};

void RTC_Init (void);

void RTC_SetTime (rtc_time_t *newTime);
void RTC_SetOpenTime (rtc_time_t *newTime);
void RTC_SetCloseTime (rtc_time_t *newTime);

void RTC_GetTime (rtc_time_t *pime);
void RTC_GetOpenTime (rtc_time_t *pTime);
void RTC_GetCloseTime (rtc_time_t *pTime);

uint8_t RTC_TestAlarm (void);

#endif /* #ifndef _RTC_H */

/* EOF */
