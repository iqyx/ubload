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

#ifndef _FW_RUNNER_H_
#define _FW_RUNNER_H_

#include <stdint.h>

struct fw_runner {
	void *base;

};


int32_t fw_runner_init(struct fw_runner *r, void *base);
#define FW_RUNNER_INIT_OK 0
#define FW_RUNNER_INIT_FAILED -1

int32_t fw_runner_jump(struct fw_runner *r);
#define FW_RUNNER_JUMP_OK 0
#define FW_RUNNER_JUMP_FAILED -1

int32_t fw_runner_reset(struct fw_runner *r);
#define FW_RUNNER_RESET_OK 0
#define FW_RUNNER_RESET_FAILED -1


#endif

