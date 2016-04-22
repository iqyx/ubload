/*
 * Module for interfacing S1D13700 display controllers using the libopencm3 API
 * (8bit parallel interface)
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

#include "hal_module.h"
#include "interface_display.h"

#include "qdl_primitives.h"
#include "qdl_render_line.h"


struct module_s1d13700_locm3_port {
	uint32_t data_port;
	uint16_t data_port_mask;
	uint8_t data_port_shift;

	uint32_t cs_port;
	uint16_t cs_pin;

	uint32_t rd_port;
	uint16_t rd_pin;

	uint32_t wr_port;
	uint16_t wr_pin;

	uint32_t rst_port;
	uint16_t rst_pin;

	uint32_t a0_port;
	uint16_t a0_pin;
};

struct module_s1d13700_locm3 {

	struct hal_module_descriptor module;
	struct interface_display iface;

	const struct module_s1d13700_locm3_port *port;

	qdlWidget *screen;
};


int32_t module_s1d13700_locm3_init(struct module_s1d13700_locm3 *self, const char *name, const struct module_s1d13700_locm3_port *port);
#define MODULE_S1D13700_LOCM3_INIT_OK 0
#define MODULE_S1D13700_LOCM3_INIT_FAILED -1


