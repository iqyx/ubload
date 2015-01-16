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
#include <string.h>

#include <libopencm3/stm32/usart.h>

#include "config.h"
#include "config_port.h"
#include "cli.h"
#include "timer.h"
#include "lineedit.h"
#include "fw_flash.h"

static int32_t cli_print_handler(const char *s, void *ctx) {

	struct cli *c = (struct cli *)ctx;

	while (*s) {
		usart_send_blocking(c->console, *s);
		s++;
	}
	return 0;
}


static int32_t cli_prompt_callback(struct lineedit *le, void *ctx) {
	(void)ctx;

	lineedit_escape_print(le, ESC_COLOR, LINEEDIT_FG_COLOR_YELLOW);
	lineedit_escape_print(le, ESC_BOLD, 0);
	le->print_handler("uBLoad(", le->print_handler_ctx);
	lineedit_escape_print(le, ESC_DEFAULT, 0);
	le->print_handler(running_config.host, le->print_handler_ctx);
	lineedit_escape_print(le, ESC_COLOR, LINEEDIT_FG_COLOR_YELLOW);
	lineedit_escape_print(le, ESC_BOLD, 0);
	le->print_handler(") > ", le->print_handler_ctx);
	lineedit_escape_print(le, ESC_DEFAULT, 0);

	return 0;
}


int32_t cli_init(struct cli *c, uint32_t console) {
	if (c == NULL) {
		return CLI_INIT_FAILED;
	}

	c->console = console;
	lineedit_init(&(c->le), 200);
	lineedit_set_print_handler(&(c->le), cli_print_handler, (void *)c);
	lineedit_set_prompt_callback(&(c->le), cli_prompt_callback, (void *)c);

	return CLI_INIT_OK;
}


int32_t cli_free(struct cli *c) {
	if (c == NULL) {
		return CLI_FREE_FAILED;
	}

	lineedit_free(&(c->le));

	return CLI_FREE_OK;
}


int32_t cli_wait_keypress(struct cli *c) {
	if (c == NULL) {
		return CLI_WAIT_KEYPRESS_FAILED;
	}

	cli_print(c, "\r\nPress [enter] to interrupt the boot process");
	for (uint32_t i = 0; i < 20; i++) {
		if (usart_get_flag(c->console, USART_SR_RXNE)) {
			uint16_t chr = usart_recv(c->console);
			if (chr == running_config.enter_key) {
				return CLI_WAIT_KEYPRESS_ENTER;
			}
			if (chr == running_config.skip_key) {
				return CLI_WAIT_KEYPRESS_SKIP;
			}
		}
		cli_print(c, ".");
		timer_wait_ms(200);
	}

	return CLI_WAIT_KEYPRESS_SKIP;
}


int32_t cli_run(struct cli *c) {
	if (c == NULL) {
		return CLI_RUN_FAILED;
	}

	while (1) {

		/* Capture systick to measure time without keypress. */
		uint32_t last_keypress;
		timer_timeout_start(&last_keypress);

		/* Do for every single line. */
		lineedit_clear(&(c->le));
		lineedit_refresh(&(c->le));
		while (1) {
			/* TODO: insert timeout here, exit with reset on timeout. */
			/* Wait for single character or break on timeout. */
			while (!usart_get_flag(c->console, USART_SR_RXNE)) {
				if (!timer_timeout_check(last_keypress, running_config.idle_time * 1000)) {
					return CLI_RUN_TIMEOUT;
				}
			}
			uint16_t chr = usart_recv(c->console);
			timer_timeout_start(&last_keypress);

			int32_t res = lineedit_keypress(&(c->le), chr);

			if (res == LINEEDIT_ENTER) {
				break;
			}
		}

		cli_print(c, "\r\n");

		char *cmd;
		lineedit_get_line(&(c->le), &cmd);

		/* There are three special cases. Commands "reset" and "quit" do
		 * the same - reset the device with a confirmation. Command "boot"
		 * continues with the boot process (without confirmation).
		 * Appropriate value is returned in these cases. */
		if (!strcmp(cmd, "quit") || !strcmp(cmd, "reset")) {
			if (cli_confirm(c) == CLI_CONFIRM_YES) {
				return CLI_RUN_RESET;
			} else {
				continue;
			}
		}
		if (!strcmp(cmd, "boot")) {
			return CLI_RUN_BOOT;
		}

		/* TODO: parse the command here. */
		cli_execute(c, cmd);
	}
}


int32_t cli_confirm(struct cli *c) {
	if (c == NULL) {
		return CLI_CONFIRM_FAILED;
	}

	cli_print(c, "Press 'y' to confirm: ");
	uint16_t chr = usart_recv_blocking(c->console);

	if (chr == 'y' || chr == 'Y') {
		return CLI_CONFIRM_YES;
	};

	return CLI_CONFIRM_NO;
}


int32_t cli_execute(struct cli *c, char *cmd) {
	if (c == NULL || cmd == NULL) {
		return CLI_EXECUTE_FAILED;
	}

	cli_print(c, "execute = ");
	cli_print(c, cmd);
	cli_print(c, "\r\n");

	if (!strcmp(cmd, "dump")) {
		fw_flash_dump(FW_RUNNER_BASE, 0x1000);
	}

	return CLI_EXECUTE_OK;
}


int32_t cli_print(struct cli *c, char *s) {
	if (c == NULL || s == NULL) {
		return CLI_PRINT_FAILED;
	}

	cli_print_handler(s, (void *)c);

	return CLI_PRINT_OK;
}


int32_t cli_print_help(struct cli *c) {
	if (c == NULL) {
		return CLI_PRINT_HELP_FAILED;
	}

	lineedit_escape_print(&(c->le), ESC_BOLD, 0);
	cli_print(c, "[HELP] uBLoad command line interface commands:\r\n");
	lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);
	cli_print(c, "\treset     Quit the bootloader without saving changes and reset the device.\r\n");
	cli_print(c, "\tquit      Alias for <reset>\r\n");
	cli_print(c, "\tboot      Boot to user application with current settings\r\n");
	cli_print(c, "\tsave      Save settings to nonvolatile memory\r\n");
	cli_print(c, "\r\n");


	return CLI_PRINT_HELP_OK;
}
