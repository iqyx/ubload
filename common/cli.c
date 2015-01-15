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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cli.h"


int32_t cli_init(struct cli *c) {
	if (c == NULL) {
		return CLI_INIT_FAILED;
	}

	return CLI_INIT_OK;
}


int32_t cli_free(struct cli *c) {
	if (c == NULL) {
		return CLI_FREE_FAILED;
	}

	return CLI_FREE_OK;
}


int32_t cli_wait_keypress(struct cli *c) {
	if (c == NULL) {
		return CLI_WAIT_KEYPRESS_FAILED;
	}

	return CLI_WAIT_KEYPRESS_SKIP;
}
