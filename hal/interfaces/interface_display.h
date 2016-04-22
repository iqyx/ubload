/**
 * uMeshFw display interface
 *
 * Copyright (C) 2015-2016, Marek Koza, qyx@krtko.org
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

#include "hal_interface.h"
#include "qdl_primitives.h"

struct interface_display_vmt {
	int32_t (*set_screen)(void *context, qdlWidget *screen);
	void *context;
};

struct interface_display {
	struct hal_interface_descriptor descriptor;

	struct interface_display_vmt vmt;

};


int32_t interface_display_init(struct interface_display *self);
#define INTERFACE_DISPLAY_INIT_OK 0
#define INTERFACE_DISPLAY_INIT_FAILED -1

int32_t interface_display_set_screen(struct interface_display *self, qdlWidget *screen);
#define INTERFACE_DISPLAY_SET_SCREEN_OK 0
#define INTERFACE_DISPLAY_SET_SCREEN_FAILED -1




