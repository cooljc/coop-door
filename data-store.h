/*
 * Filename		: data-store.h
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
#ifndef _DATA_STORE_H
#define _DATA_STORE_H

#include "rtc.h"

void DS_GetOpenAlarm(rtc_time_t *alarm);
void DS_GetCloseAlarm(rtc_time_t *alarm);

void DS_SetOpenAlarm(rtc_time_t *alarm);
void DS_SetCloseAlarm(rtc_time_t *alarm);

#endif /* #ifndef _DATA_STORE_H */
/* EOF */
