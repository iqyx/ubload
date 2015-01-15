/**
 * uBLoad firmware runner
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
#include <stdlib.h>
#include <stdio.h>

#include "fw_runner.h"


int32_t fw_runner_init(struct fw_runner *r, void *base) {
	if (r == NULL) {
		return FW_RUNNER_INIT_FAILED;
	}

	r->base = base;

	return FW_RUNNER_INIT_OK;
}


int32_t fw_runner_jump(struct fw_runner *r) {
	if (r == NULL) {
		return FW_RUNNER_JUMP_FAILED;
	}

	register uint32_t msp __asm("msp");
	typedef void (*t_app_entry)(void);

	/* application vector table is positioned at the starting address +
	 * size of the header (1KB) */
	const uint32_t *vector_table = (uint32_t *)(r->base + 0x400);

	/* load application entry point to app_entry function pointer */
	t_app_entry app_entry = (t_app_entry)(vector_table[1]);

	/* set app stack pointer and jump to application */
	msp = vector_table[0];
	app_entry();
	(void)msp;

	/* never reached */
	return FW_RUNNER_JUMP_OK;
}


int32_t fw_runner_reset(struct fw_runner *r) {
	if (r == NULL) {
		return FW_RUNNER_RESET_FAILED;
	}

	/* TODO: remove this magic. */
        *((unsigned long*)0xE000ED0C) = 0x05FA0004;
        while (1) {
		;
	}

	/* Unreachable. */
	return FW_RUNNER_RESET_OK;
}
