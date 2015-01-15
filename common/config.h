/**
 * uBLoad configuration
 *
 * Copyright (C) 2015, Marek Koza, qyx@krtko.org
 *
 * This file is part of uMesh node firmware (http://qyx.krtko.org/projects/umesh)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * This file contains all configuration variables which are supported by uBLoad.
 * Config options are initialized during the bootloader initialization and can
 * be later overriden with values from configuration memory (eg. EEPROM).
 *
 * Not all uBLoad ports read and use all provided values.
 */


/**
 * Mode of onboard status/diagnostic LEDs.
 */
enum ubload_config_led_mode {
	/**
	 * All status LEDs are off during bootloader code execution
	 */
	LED_MODE_OFF,
	/**
	 * Status LED is lit during the whole bootloading process. It is
	 * switched off when the bootloader jumps to the user application.
	 */
	LED_MODE_STILL_ON,
	/**
	 * Status LED(s) blinks according to actual bootloading phase.
	 * Recommended blink sequences are provided in the led.h file.
	 */
	LED_MODE_BASIC,
	/**
	 * Multiple status LEDs are initialized (if the board has them) and
	 * used to provide boot process feedback to the user. Usually all
	 * available LEDs with different colors (or multicolor LEDs) are used.
	 */
	LED_MODE_DIAG
};


struct ubload_config {

	/* Console serial port settings */
	bool serial_enabled;
	uint32_t serial_speed;

	/* Diagnostic LED settings */
	enum ubload_config_led_mode led_mode;

	/* Bootloader enter setup */
	bool cli_enabled;
	uint8_t wait_time;
	char enter_key;
	char skip_key;

};


extern const struct ubload_config default_config;
extern struct ubload_config running_config;

#endif


