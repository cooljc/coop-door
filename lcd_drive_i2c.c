/*
 * Filename		: lcd_drive_i2c.c
 * Author		: Jon Cross
 * Date			: 05/10/2014
 * Description	: I2C driver for 2x16 LCD display
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
#include <util/delay.h>
#include "i2cmaster.h"
#include "lcd-driver.h"
#include "rtc.h"

/* ------------------------------------------------------------------ *
 *
 * I2C Address
 * 0100 1110 = 0x4E
 * 0010 0111 = 0x27

 * PINs
 * P0 - RS
 * P1 - RW
 * P2 - E
 * P3 - Backlight
 * P4 - D4
 * P5 - D5
 * P6 - D6
 * P7 - D7
 *
 * ------------------------------------------------------------------ */
#define BL 3  // Backlight
#define EN 2  // Enable bit
#define RW 1  // Read/Write bit
#define RS 0  // Register select bit

#define D4 4
#define D5 5
#define D6 6
#define D7 7

/* ------------------------------------------------------------------ */
/* Commands */
/* ------------------------------------------------------------------ */
#define CMD_CLEAR_DISPLAY			0x01
#define CMD_RETURN_HOME				0x02
#define CMD_ENTRY_MODE				0x04
#define CMD_DISPLAY_CONTROL			0x08
#define CMD_CURSOR_DISPLAY_SHIFT	0x10
#define CMD_FUNCTION_SET			0x20
#define CMD_DDRAM_SET				0x80

/* ------------------------------------------------------------------ */
/* Options */
/* ------------------------------------------------------------------ */
#define OPT_INCREMENT				0x02	// CMD_ENTRY_MODE
#define OPT_ENTRY_DISPLAY_SHIFT		0x01	// CMD_ENTRY_MODE
#define OPT_ENABLE_DISPLAY			0x04	// CMD_DISPLAY_CONTROL
#define OPT_ENABLE_CURSOR			0x02	// CMD_DISPLAY_CONTROL
#define OPT_ENABLE_BLINK			0x01	// CMD_DISPLAY_CONTROL
#define OPT_DISPLAY_SHIFT			0x08	// CMD_CURSOR_DISPLAY_SHIFT
#define OPT_SHIFT_RIGHT				0x04	// CMD_CURSOR_DISPLAY_SHIFT 0 = Left
#define OPT_2_LINES					0x08	// CMD_FUNCTION_SET 0 = 1 line
#define OPT_5X10_DOTS				0x04	// CMD_FUNCTION_SET 0 = 5x7 dots
	
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
enum
{
	COMMAND = 0,
	DATA
};

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
uint8_t devAddr = 0x4e;
uint8_t backlight = 0;
const char ascii[10] = "0123456789";

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static void lcd_write_ioex (uint8_t data)
{
	i2c_start (devAddr);
	i2c_write (data);
	i2c_stop ();
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static void lcd_strobe (uint8_t byte)
{
	uint8_t data = byte;
	if (backlight) {
		data |= (1 << BL);
	}
	
	lcd_write_ioex (data);
	lcd_write_ioex (data | (1 << EN));
	_delay_ms(2);
}

/* ------------------------------------------------------------------ *
 *
 * byte = byte to send
 * mode = command (0)/data (1)
 *
 * ------------------------------------------------------------------ */
static void lcd_command (uint8_t byte, uint8_t mode)
{
	uint8_t port_bytes[2];
	uint8_t nibble;
	
	port_bytes[0] = (byte & 0xf0);
	port_bytes[1] = ((byte << 4) & 0xf0);
	for (nibble = 0; nibble < 2; nibble++) {
		if (mode == DATA) {
			port_bytes[nibble] |= (1 << RS);
		}
		// strobe E
		lcd_strobe (port_bytes[nibble]);
	}  
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
static void lcd_setPosition(uint8_t line, uint8_t pos)
{
	uint8_t address = 0;
	if (line == 1)
		address = pos;
	else if (line == 2)
		address = 0x40 + pos;
	else if (line == 3)
		address = 0x14 + pos;
	else if (line == 4)
		address = 0x54 + pos;
	lcd_command (CMD_DDRAM_SET + address, COMMAND);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_Init(void)
{
	uint8_t initialize_i2c_data = 0;

	// Set initial values to 0
	lcd_write_ioex (0);
	
	// Activate LCD
	initialize_i2c_data = (1 << D4) | (1 << D5);
	lcd_strobe (initialize_i2c_data);
	lcd_strobe (initialize_i2c_data);
	lcd_strobe (initialize_i2c_data);

	// initialise LCD for 4bit mode
	initialize_i2c_data &= ~(1 << D4);
	lcd_strobe (initialize_i2c_data);
	
	// set two line
	lcd_command (CMD_FUNCTION_SET | OPT_2_LINES, COMMAND);
	lcd_command (CMD_DISPLAY_CONTROL | OPT_ENABLE_DISPLAY, COMMAND);
	lcd_command (CMD_CLEAR_DISPLAY, COMMAND);
	lcd_command (CMD_ENTRY_MODE | OPT_INCREMENT, COMMAND);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_WriteLine(uint8_t line, uint8_t len, char *str)
{
	uint8_t loop = 0;
	lcd_setPosition (line+1, 0);
	for (loop=0; loop<len; loop++) {
		lcd_command (str[loop], DATA);
	}
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_SetBacklight(uint8_t onNotOff)
{
	uint8_t byte = 0;
	backlight = onNotOff;
	if (onNotOff)
		byte = (1 << BL);
	lcd_write_ioex (byte);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
#ifdef CLOCK_SHOW_SECONDS
void LCD_WriteTime(rtc_time_t currentTime)
{
	uint8_t n1, n2;
	lcd_setPosition (1, 4);
	
	// Hour
	n1 = (currentTime.m_hour / 10);
	n2 = (currentTime.m_hour % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command (':', DATA);

	// Minute
	n1 = (currentTime.m_min / 10);
	n2 = (currentTime.m_min % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command (':', DATA);

	// Second
	n1 = (currentTime.m_sec / 10);
	n2 = (currentTime.m_sec % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	
	lcd_command (CMD_RETURN_HOME, COMMAND); // return home
}
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_SetCursor(uint8_t state)
{
	if (state == LCD_CURSOR_OFF) {
		// turn cursor off
		lcd_command (CMD_DISPLAY_CONTROL, COMMAND);
		lcd_command (CMD_DISPLAY_CONTROL | OPT_ENABLE_DISPLAY, COMMAND);
	}
	else {
		switch (state) {
			case LCD_CURSOR_HOUR:
				// set cursor to hour
				lcd_setPosition (1, 5);
				break;
			case LCD_CURSOR_MIN:
				// set cursot to minute
				lcd_setPosition (1, 8);
				break;
			case LCD_CURSOR_DAYNAME:
				// set cursot to day name
				lcd_setPosition (2, 1);
				break;
			case LCD_CURSOR_DAY:
				// set cursot to day
				lcd_setPosition (2, 6);
				break;
			case LCD_CURSOR_MONTH:
				// set cursot to month
				lcd_setPosition (2, 9);
			case LCD_CURSOR_YEAR:
				// set cursot to year
				lcd_setPosition (2, 14);
				break;
		}
		lcd_command (CMD_DISPLAY_CONTROL
					| OPT_ENABLE_DISPLAY
					| OPT_ENABLE_CURSOR
					| OPT_ENABLE_BLINK, COMMAND);
	}
}
#else
void LCD_WriteTime(rtc_time_t currentTime)
{
	uint8_t n1, n2;
	lcd_setPosition (1, 5);

	// -----xx:xx------
	// Hour
	n1 = (currentTime.m_hour / 10);
	n2 = (currentTime.m_hour % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command (':', DATA);

	// Minute
	n1 = (currentTime.m_min / 10);
	n2 = (currentTime.m_min % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);

	lcd_command (CMD_RETURN_HOME, COMMAND); // return home
}
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_SetCursor(uint8_t state)
{
	if (state == LCD_CURSOR_OFF) {
		// turn cursor off
		lcd_command (CMD_DISPLAY_CONTROL, COMMAND);
		lcd_command (CMD_DISPLAY_CONTROL | OPT_ENABLE_DISPLAY, COMMAND);
	}
	else {
		switch (state) {
			case LCD_CURSOR_HOUR:
				// set cursor to hour
				lcd_setPosition (1, 5);
				break;
			case LCD_CURSOR_MIN:
				// set cursot to minute
				lcd_setPosition (1, 8);
				break;
			case LCD_CURSOR_DAYNAME:
				// set cursot to day name
				lcd_setPosition (2, 1);
				break;
			case LCD_CURSOR_DAY:
				// set cursot to day
				lcd_setPosition (2, 6);
				break;
			case LCD_CURSOR_MONTH:
				// set cursot to month
				lcd_setPosition (2, 9);
			case LCD_CURSOR_YEAR:
				// set cursot to year
				lcd_setPosition (2, 14);
				break;
		}
		lcd_command (CMD_DISPLAY_CONTROL
					| OPT_ENABLE_DISPLAY
					| OPT_ENABLE_CURSOR
					| OPT_ENABLE_BLINK, COMMAND);
	}
}
#endif

#ifdef DS1307_BOARD
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
const char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
void LCD_WriteDate(rtc_date_t currentDate)
{
	uint8_t n1, n2;
	/* "----------------" */
	/* " DDD-dd/mm/yyyy " */
	lcd_setPosition (2, 1);

	/* Day Name */
	lcd_command (days[currentDate.m_dayNumber-1][0], DATA);
	lcd_command (days[currentDate.m_dayNumber-1][1], DATA);
	lcd_command (days[currentDate.m_dayNumber-1][2], DATA);
	lcd_command ('-', DATA);
	
	/* Day */
	n1 = (currentDate.m_day / 10);
	n2 = (currentDate.m_day % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command ('/', DATA);
	
	/* Month */
	n1 = (currentDate.m_month / 10);
	n2 = (currentDate.m_month % 10);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command ('/', DATA);
	
	/* Year */
	n1 = (currentDate.m_year / 10);
	n2 = (currentDate.m_year % 10);
	lcd_command ('2', DATA);
	lcd_command ('0', DATA);
	lcd_command (ascii[n1], DATA);
	lcd_command (ascii[n2], DATA);
	lcd_command ('/', DATA);

	lcd_command (CMD_RETURN_HOME, COMMAND); // return home
}
#endif

void LCD_Off(void)
{
#if 0
	lcd_write(0xFE);
	lcd_write(0x08);
#endif
}
