/**
 * uBLoad logging services
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

#include "u_log.h"
#include "u_assert.h"
#include "system_log.h"
#include "config.h"
#include "config_port.h"
#include "cli.h"


#if PORT_CLOG == true
	struct log_cbuffer *system_log;
#endif


int32_t u_log_init(void) {

	uint8_t *clog_pos = (void *)PORT_CLOG_BASE;
	system_log = (struct log_cbuffer *)clog_pos;
	uint8_t *log_data = (uint8_t *)(clog_pos + sizeof(struct log_cbuffer));

	log_cbuffer_init(system_log, log_data, PORT_CLOG_SIZE - sizeof(struct log_cbuffer *));

	return U_LOG_INIT_OK;
}


/**
 * u_assert macro from u_assert.h is using this function to output assertion
 * failure messages. If system circular log is enabled, redirect them there.
 *
 * TODO: should be moved to u_assert.c
 */
#if PORT_CLOG == true

	int u_assert_func(const char *expr, const char *fname, int line) {

		if (system_log != NULL) {
			u_log(system_log, LOG_TYPE_ASSERT,
				"Assertion '%s' failed at %s:%d", expr, fname, line);
		}
		return 1;
	}
#else
	int u_assert_func(const char *expr, const char *fname, int line) {
		(void)expr;
		(void)fname;
		(void)line;
		return 1;
	}
#endif


static void system_log_print_handler(struct log_cbuffer *buf, uint32_t pos, void *ctx) {
	(void)buf;
	(void)pos;

	struct cli *c = (struct cli *)ctx;

	cli_print(c, "test - log message added\r\n");
}


int32_t u_log_set_cli_print_handler(struct cli *c) {
	#if PORT_CLOG == true
		log_cbuffer_set_print_handler(system_log, system_log_print_handler, (void *)c);
	#endif

	return U_LOG_SET_CLI_PRINT_HANDLER_OK;
}
