/*
 * Filename		: button-driver.h
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
#ifndef _BUTTON_DRIVER_H
#define _BUTTON_DRIVER_H

/* available keys */
enum {
	KEY_NONE = 0,
	KEY_OPEN,		/* Open Key */
	KEY_CLOSE,		/* Close Key */
	KEY_DOOR_OPEN,	/* Door Open Reed Switch */
	KEY_DOOR_CLOSED,/* Door Closed Reed Switch */	
	KEY_MENU		/* Menu Key */
};

/* Public Functions */
void    BUTTON_Init(void);
uint8_t BUTTON_GetKey(void);

#endif /* _BUTTON_DRIVER_H */

/* EOF */
