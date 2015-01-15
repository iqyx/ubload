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

#ifndef _TIMER_H_
#define _TIMER_H_

void sys_tick_handler(void);

extern volatile uint32_t systick_counter;

int32_t timer_init(void);
#define TIMER_INIT_OK 0
#define TIMER_INIT_FAILED -1

int32_t timer_free(void);
#define TIMER_FREE_OK 0
#define TIMER_FREE_FAILED -1

int32_t timer_wait_ms(uint32_t delay);
#define TIMER_WAIT_MS_OK 0
#define TIMER_WAIT_MS_FAILED -1


#endif


