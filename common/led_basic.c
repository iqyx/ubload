/**
 * uBLoad basic status led driver
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

#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/stm32/gpio.h>

#include "led_basic.h"


int32_t led_basic_init(struct led_basic *led, uint32_t port, uint8_t pin, bool inv) {

	led->port = port;
	led->pin = pin;
	led->inv = inv;
	led->state = false;

	gpio_mode_setup(port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, (1 << pin));
	led_basic_set(led, false);

	return LED_BASIC_INIT_OK;
}


int32_t led_basic_free(struct led_basic *led) {
	(void)led;

	return LED_BASIC_FREE_OK;
}


int32_t led_basic_timer(struct led_basic *led) {

	led_basic_set(led, !led->state);

	return LED_BASIC_TIMER_OK;
}

int32_t led_basic_set(struct led_basic *led, bool led_state) {

	led->state = led_state;
	if (led_state != led->inv) {
		/* If the led state is true and it should not be inverted or
		 * if the led state is false and it should be inverted,
		 * turn it on */
		gpio_set(led->port, (1 << led->pin));
	} else {
		/* Otherwise turn it off */
		gpio_clear(led->port, (1 << led->pin));
	}

	return LED_BASIC_SET_OK;
}
