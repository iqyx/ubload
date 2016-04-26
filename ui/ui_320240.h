/*
 * uBLoad user interface for 320x240 pixel displays
 *
 * Copyright (C) 2016, Marek Koza, qyx@krtko.org
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

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "interface_display.h"

/** @todo configuration */


struct ui_320240 {

	struct interface_display *display;
	/** @todo event manager dependency */
};


int32_t ui_320240_init(struct ui_320240 *self, struct interface_display *display);
#define UI_320240_INIT_OK 0
#define UI_320240_INIT_FAILED -1


