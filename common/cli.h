/**
 * uBLoad command line interface
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

#ifndef _CLI_H_
#define _CLI_H_

#include <stdint.h>
#include <stdbool.h>

struct cli {


};


int32_t cli_init(struct cli *c);
#define CLI_INIT_OK 0
#define CLI_INIT_FAILED -1

int32_t cli_free(struct cli *c);
#define CLI_FREE_OK 0
#define CLI_FREE_FAILED -1

int32_t cli_wait_keypress(struct cli *c);
#define CLI_WAIT_KEYPRESS_ENTER 1
#define CLI_WAIT_KEYPRESS_SKIP 0
#define CLI_WAIT_KEYPRESS_FAILED -1


#endif

