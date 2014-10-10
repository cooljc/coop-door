/*
 * Filename		: lcd-driver.h
 * Author		: Jon Cross
 * Date			: 17/08/2012
 * Description	: Serial bit banger for 2x16 LCD display
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

#ifndef _LCD_DRIVER_H
#define _LCD_DRIVER_H

#include "rtc.h"

enum {
	LCD_CURSOR_OFF = 0,
	LCD_CURSOR_HOUR,
	LCD_CURSOR_MIN,
	LCD_CURSOR_DAYNAME,
	LCD_CURSOR_DAY,
	LCD_CURSOR_MONTH,
	LCD_CURSOR_YEAR
};

void LCD_Init(void);
void LCD_WriteLine(uint8_t line, uint8_t len, char *str);
void LCD_WriteTime(rtc_time_t currentTime);
void LCD_SetCursor(uint8_t state);
void LCD_Off(void);
void LCD_SetBacklight(uint8_t onNotOff);
uint8_t LCD_GetBacklight (void);
#ifdef DS1307_BOARD
void LCD_WriteDate(rtc_date_t currentDate);
#endif /* #ifdef DS1307_BOARD */

#endif /* #ifndef _LCD_DRIVER_H */
