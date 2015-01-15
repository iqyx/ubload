/**
 * uBLoad timer and delay routines
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
#include <stdlib.h>

#include <libopencm3/cm3/systick.h>

#include "timer.h"

volatile uint32_t systick_counter = 0;

void sys_tick_handler(void) {
	systick_counter++;
}


int32_t timer_init(void) {
	/* Init systick (1000Hz) */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(15999);
	systick_interrupt_enable();
	systick_counter_enable();

	return TIMER_INIT_OK;
}


int32_t timer_free(void) {
	systick_counter_disable();
	systick_interrupt_disable();

	return TIMER_FREE_OK;
}


int32_t timer_wait_ms(uint32_t delay) {

	/* capture actual systick counter and add the wanted delay */
	uint32_t start = systick_counter;
	uint32_t stop = start + delay;

	while ((start <= stop && systick_counter >= start && systick_counter < stop) ||
	       (start > stop && (systick_counter >= start || systick_counter < stop))) {
		;
	}

	return TIMER_WAIT_MS_OK;
}
