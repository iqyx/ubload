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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include "FreeRTOS.h"
#include "port.h"
#include "task.h"
#include "u_assert.h"
#include "u_log.h"

#include "hal_module.h"
#include "interface_display.h"
#include "s1d13700_locm3.h"

#include "qdl_primitives.h"
#include "qdl_render_line.h"


static int32_t module_s1d13700_locm3_set_screen(void *context, qdlWidget *screen) {
	if (u_assert(context != NULL)) {
		return INTERFACE_DISPLAY_SET_SCREEN_FAILED;
	}
	struct module_s1d13700_locm3 *self = (struct module_s1d13700_locm3 *)context;

	self->screen = screen;

	return INTERFACE_DISPLAY_SET_SCREEN_OK;
}


static inline void port_write(struct module_s1d13700_locm3 *self, uint8_t byte, bool command) {
	if (command) {
		gpio_set(self->port->a0_port, self->port->a0_pin);
	} else {
		gpio_clear(self->port->a0_port, self->port->a0_pin);
	}
	gpio_clear(self->port->data_port, self->port->data_port_mask);
	gpio_set(GPIOE, byte << self->port->data_port_shift);

	gpio_clear(self->port->cs_port, self->port->cs_pin);
	gpio_clear(self->port->wr_port, self->port->wr_pin);
	gpio_set(self->port->wr_port, self->port->wr_pin);
	gpio_set(self->port->cs_port, self->port->cs_pin);

	if (command) {
		vTaskDelay(1);
	}
}


static inline void display_clear(struct module_s1d13700_locm3 *self) {
	/* Set cursor to the start */
	port_write(self, 0x46, true);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);

	/* write data */
	port_write(self, 0x42, true);
	for (uint32_t i = 0; i < 9600; i++) {
		port_write(self, 0x00, false);
	}
}


static inline void display_refresh(struct module_s1d13700_locm3 *self) {
	if (u_assert(self != NULL)) {
		return;
	}
	if (self->screen == NULL) {
		return;
	}

	port_write(self, 0x46, true);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);

	qdlColor c[320];
	port_write(self, 0x42, true);
	for (int y = 0; y < 240; y++) {
		for (int x = 0; x < 320; x++) {
			c[x] = (qdlColor)COLOR_WHITE;
		}

		qdl_render_line(c, 0, 320 - 1, y, self->screen);

		for (int x = 0; x < 40; x++) {
			uint32_t byte = 0;
			for (int xx = 0; xx < 8; xx++) {
				byte = byte << 1;
				if (c[x * 8 + xx].r < 128) {
					byte += 1;
				}
			}
			port_write(self, byte, false);
		}
	}
}


static void module_s1d13700_locm3_task(void *p) {
	struct module_s1d13700_locm3 *self = (struct module_s1d13700_locm3 *)p;

	while (1) {
		display_refresh(self);
		vTaskDelay(1);
	}
}


int32_t module_s1d13700_locm3_init(struct module_s1d13700_locm3 *self, const char *name, const struct module_s1d13700_locm3_port *port) {
	if (u_assert(self != NULL)) {
		return MODULE_S1D13700_LOCM3_INIT_FAILED;
	}

	memset(self, 0, sizeof(struct module_s1d13700_locm3));
	hal_module_descriptor_init(&(self->module), name);
	hal_module_descriptor_set_shm(&(self->module), (void *)self, sizeof(struct module_s1d13700_locm3));

	self->port = port;

	interface_display_init(&(self->iface));
	self->iface.vmt.context = (void *)self;
	self->iface.vmt.set_screen = module_s1d13700_locm3_set_screen;

	/* Ininitialze the required GPIO pins */
	gpio_mode_setup(port->data_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->data_port_mask);
	gpio_set_output_options(port->data_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->data_port_mask);

	gpio_mode_setup(port->cs_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->cs_pin);
	gpio_set_output_options(port->cs_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->cs_pin);

	gpio_mode_setup(port->rd_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->rd_pin);
	gpio_set_output_options(port->rd_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->rd_pin);

	gpio_mode_setup(port->wr_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->wr_pin);
	gpio_set_output_options(port->wr_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->wr_pin);

	gpio_mode_setup(port->rst_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->rst_pin);
	gpio_set_output_options(port->rst_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->rst_pin);

	gpio_mode_setup(port->a0_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, port->a0_pin);
	gpio_set_output_options(port->a0_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, port->a0_pin);

	/* Set default GPIO pin states. */
	gpio_set(port->rst_port, port->rst_pin);
	gpio_set(port->rd_port, port->rd_pin);
	gpio_set(port->wr_port, port->wr_pin);
	gpio_set(port->cs_port, port->cs_pin);
	gpio_clear(port->a0_port, port->a0_pin);

	/* All GPIO pins are ready, we can reset the display now. */
	vTaskDelay(10);
	gpio_clear(port->rst_port, port->rst_pin);
	vTaskDelay(10);
	gpio_set(port->rst_port, port->rst_pin);
	vTaskDelay(10);

	/* Initialize the display. */
	port_write(self, 0x40, true);  /* System setup */
	port_write(self, 0x30, false);
	port_write(self, 0x87, false); /* Character width = 8 */
	port_write(self, 0x07, false); /* Character height = 8 */
	port_write(self, 0x27, false); /* CR */
	port_write(self, 0x2f, false); /* T/CR */
	port_write(self, 0xef, false); /* 240 lines */
	port_write(self, 0x28, false); /* Horizontal address range LSB (APL) */
	port_write(self, 0x00, false); /* Horizontal address range MSB (APH) */

	port_write(self, 0x44, true);  /* Scroll area setup */
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 239, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);
	port_write(self, 0x00, false);

	port_write(self, 0x5a, true);
	port_write(self, 0x00, false);

	port_write(self, 0x51, true);
	port_write(self, 0x09, false);

	port_write(self, 0x60, true);
	port_write(self, 0x00, false);

	/* Turn the display ON */
	port_write(self, 0x59, true);
	port_write(self, 0x14, false);

	display_clear(self);

	/* Create a periodic display refresh task */
	xTaskCreate(module_s1d13700_locm3_task, "s1d13700", configMINIMAL_STACK_SIZE + 1024, (void *)self, 1, NULL);

	u_log(system_log, LOG_TYPE_INFO, "%s: S1D13700 display module initialized", self->module.name);

	return MODULE_S1D13700_LOCM3_INIT_OK;
}



