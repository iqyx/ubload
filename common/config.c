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

#include "config.h"


struct ubload_config running_config;
const struct ubload_config default_config = {

	.serial_enabled = true,
	.serial_speed = 115200,
	.led_mode = LED_MODE_BASIC,
	.cli_enabled = true,
	.enter_key = 13,
	.skip_key = 27,
	.wait_time = 5,
	.idle_time = 300,
	.host = "unknown",
	.watchdog_enabled = false,

};

