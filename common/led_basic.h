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

#ifndef _LED_BASIC_H_
#define _LED_BASIC_H_

#include <stdint.h>
#include <stdbool.h>


struct led_basic {
	uint32_t port;
	uint8_t pin;
	bool inv;
	bool state;
};


int32_t led_basic_init(struct led_basic *led, uint32_t port, uint8_t pin, bool inv);
#define LED_BASIC_INIT_OK 0
#define LED_BASIC_INIT_FAILED -1

int32_t led_basic_free(struct led_basic *led);
#define LED_BASIC_FREE_OK 0
#define LED_BASIC_FREE_FAILED -1

int32_t led_basic_timer(struct led_basic *led);
#define LED_BASIC_TIMER_OK 0
#define LED_BASIC_TIMER_FAILED -1

int32_t led_basic_set(struct led_basic *led, bool led_state);
#define LED_BASIC_SET_OK 0
#define LED_BASIC_SET_FAILED -1


#endif

