/*
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "u_assert.h"
#include "hal_interface.h"
#include "interface_display.h"

#include "qdl_primitives.h"


int32_t interface_display_init(struct interface_display *self) {
	if (u_assert(self != NULL)) {
		return INTERFACE_DISPLAY_INIT_FAILED;
	}

	memset(self, 0, sizeof(struct interface_display));
	hal_interface_init(&(self->descriptor), HAL_INTERFACE_TYPE_DISPLAY);

	return INTERFACE_DISPLAY_INIT_OK;
}


int32_t interface_display_set_screen(struct interface_display *self, qdlWidget *screen) {
	if (u_assert(self != NULL)) {
		return INTERFACE_DISPLAY_SET_SCREEN_FAILED;
	}
	if (self->vmt.set_screen != NULL) {
		return self->vmt.set_screen(self->vmt.context, screen);
	}
	return INTERFACE_DISPLAY_SET_SCREEN_FAILED;
}


