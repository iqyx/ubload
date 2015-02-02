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
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "config.h"
#include "config_port.h"
#include "u_log.h"
#include "u_assert.h"
#include "cli.h"
#include "cli_cmd.h"
#include "timer.h"
#include "lineedit.h"
#include "fw_image.h"


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

	le->print_handler(ESC_COLOR_FG_YELLOW ESC_BOLD "uBLoad(" ESC_DEFAULT, le->print_handler_ctx);
	le->print_handler(running_config.host, le->print_handler_ctx);
	le->print_handler(ESC_COLOR_FG_YELLOW ESC_BOLD ") > " ESC_DEFAULT, le->print_handler_ctx);

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

		uint32_t argc = 0;
		char *argv[CLI_MAX_ARGC];
		cli_parse_command(c, cmd, &argc, argv, CLI_MAX_ARGC);
		cli_execute(c, argc, argv);
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


int32_t cli_progress_callback(uint32_t progress, uint32_t total, void *ctx) {
	struct cli *c = (struct cli *)ctx;

	char s[40];

	cli_print(c, "\r");
	//~ lineedit_escape_print(&(c->le), ESC_ERASE_LINE_END, 0);
	snprintf(s, sizeof(s), "progress %3u%%",(unsigned int)(progress * 100 / total));
	cli_print(c, s);
	cli_print(c, " [");
	for (uint32_t i = 0; i <= 72; i++) {
		if (i <= (progress * 72 / total)) {
			cli_print(c, "#");
		} else {
			cli_print(c, " ");
		}
	}
	cli_print(c, "]");

	if (progress == total) {
		cli_print(c, "\r\n");
	}

	return FW_IMAGE_PROGRESS_CALLBACK_OK;
}


int32_t cli_parse_command(struct cli *c, char *cmd, uint32_t *argc, char *argv[], uint32_t max_argc) {
	if (u_assert(c != NULL) ||
	    u_assert(cmd != NULL) ||
	    u_assert(argc != NULL) ||
	    u_assert(argv != NULL) ||
	    u_assert(max_argc > 0)) {
		return CLI_PARSE_COMMAND_FAILED;
	}

	/* Loop up to the maximum number of accepted arguments. */
	*argc = 0;
	while (*argc < max_argc) {
		/* Eat all whitespaces first. */
		while ((*cmd == ' ') && *cmd != '\0') {
			cmd++;
		}
		if (*cmd == '\0') {
			/* Return if end of string was reached. */
			return CLI_PARSE_COMMAND_OK;
		} else {
			/* Another argument follows. Save its position and
			 * increment argument count. */
			argv[*argc] = cmd;
			(*argc)++;
		}
		/* Move to the current argument end. */
		while (*cmd != ' ') {
			if (*cmd == '\0') {
				return CLI_PARSE_COMMAND_OK;
			}

			cmd++;
		}
		/* And terminate it properly. */
		*cmd = '\0';
		cmd++;
	}

	return CLI_PARSE_COMMAND_OK;
}


int32_t cli_execute(struct cli *c, uint32_t argc, char *argv[]) {
	if (u_assert(c != NULL)) {
		return CLI_EXECUTE_FAILED;
	}
	if (argc == 0) {
		return CLI_EXECUTE_OK;
	}

	/* At least one argument is given. */
	if (!strcmp(argv[0], "dump")) {
		if (argc == 1) {
			cli_print(c, "Required argument is missing (<filename>, xmodem, console)\r\n");
		} else {
			if (!strcmp(argv[1], "xmodem")) {
				cli_cmd_dump_xmodem(c);
			} else if (!strcmp(argv[1], "console")) {
				if (argc < 4) {
					cli_print(c, "Required argument is missing.\r\n");
				} else {
					uint32_t origin = atoi(argv[2]);
					uint32_t length = atoi(argv[3]);
					cli_cmd_dump_console(c, FW_IMAGE_BASE + origin, length);
				}
			} else {
				cli_cmd_dump_file(c, argv[1]);
			}

		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "help")) {
		cli_cmd_help(c);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "erase")) {
		cli_cmd_erase(c);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "program")) {
		if (argc == 1) {
			cli_print(c, "Required argument is missing.\r\n");
		}

		if (argc == 2) {
			if (!strcmp(argv[1], "xmodem")) {
				cli_cmd_program_xmodem(c);
			} else {
				cli_cmd_program_file(c, argv[1]);
			}

		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "fs")) {

		if (argc == 1) {
			cli_print(c, "Required argument is missing (download, upload, delete, format)\r\n");
		} else {
			if (!strcmp(argv[1], "download")) {
				if (argc < 3) {
					cli_print(c, "Filename missing\r\n");
				} else {
					cli_cmd_fs_download(c, argv[1]);
				}
			}
			if (!strcmp(argv[1], "upload")) {
				if (argc < 3) {
					cli_print(c, "Filename missing\r\n");
				} else {
					cli_cmd_fs_upload(c, argv[1]);
				}
			}
			if (!strcmp(argv[1], "delete")) {
				if (argc < 3) {
					cli_print(c, "Filename missing\r\n");
				} else {
					cli_cmd_fs_delete(c, argv[1]);
				}
			}
			if (!strcmp(argv[1], "format")) {
				cli_cmd_fs_format(c);
			}
		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "config")) {

		if (argc == 1) {
			cli_print(c, "Required argument is missing (set, print, save, load, default)\r\n");
		} else {
			if (!strcmp(argv[1], "set")) {
				if (argc < 4) {
					cli_print(c, "Config key or value is missing.\r\n");
				} else {
					cli_cmd_config_set(c, argv[2], argv[3]);
				}
			}
			if (!strcmp(argv[1], "print")) {
				if (argc < 3) {
					cli_cmd_config_print_all(c);
				} else {
					cli_cmd_config_print_key(c, argv[2]);
				}
			}
			if (!strcmp(argv[1], "save")) {
				cli_cmd_config_save(c);
			}
			if (!strcmp(argv[1], "load")) {
				cli_cmd_config_load(c);
			}
			if (!strcmp(argv[1], "default")) {
				cli_cmd_config_default(c);
			}
		}
		return CLI_EXECUTE_OK;
	}


	if (!strcmp(argv[0], "verify")) {
		if (argc == 1) {
			cli_cmd_verify_flash(c);
		}
		if (argc == 2) {
			cli_print(c, "Verification of a saved firmware image is not supported yet.\r\n");
		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "authenticate")) {
		if (argc == 1) {
			cli_cmd_authenticate_flash(c);
		}
		if (argc == 2) {
			cli_print(c, "Authentication of a saved firmware image is not supported yet.\r\n");
		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "pubkey")) {
		if (argc <= 1) {
			cli_print(c, "Required argument is missing.\r\n");
			return CLI_EXECUTE_OK;
		} else {
			if (!strcmp(argv[1], "print")) {
				cli_cmd_pubkey_print(c);
			}
			if (!strcmp(argv[1], "add")) {
				if (argc <= 2) {
					cli_print(c, "Key is missing.\r\n");
				} else {
					cli_cmd_pubkey_add(c, argv[2]);
				}
			}
			if (!strcmp(argv[1], "lock")) {
				if (argc <= 2) {
					cli_print(c, "Slot number is missing.\r\n");
				} else {
					if (!strcmp(argv[2], "all")) {
						cli_cmd_pubkey_lock(c, UINT32_MAX);
					} else {
						uint32_t slot_num = atoi(argv[2]);
						cli_cmd_pubkey_lock(c, slot_num);
					}
				}
			}
		}
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "log")) {
		if (argc == 1) {
			cli_print(c, "Required argument is missing (print)\r\n");
		} else {
			if (!strcmp(argv[1], "print")) {
				cli_cmd_log_print(c);
			}
		}
		return CLI_EXECUTE_OK;
	}

	cli_print(c, "Unknown command '");
	cli_print(c, argv[0]);
	cli_print(c, "'\r\n");

	return CLI_EXECUTE_FAILED;
}


int32_t cli_print(struct cli *c, const char *s) {
	if (c == NULL || s == NULL) {
		return CLI_PRINT_FAILED;
	}

	cli_print_handler(s, (void *)c);

	return CLI_PRINT_OK;
}


int32_t cli_print_banner(struct cli *c) {
	if (c == NULL) {
		return CLI_PRINT_BANNER_FAILED;
	}

	cli_print(c, ESC_BOLD ESC_COLOR_FG_YELLOW "\r\n### ");
	cli_print(c, PORT_BANNER);
	cli_print(c, ", ");
	cli_print(c, PORT_NAME);
	cli_print(c, " platform\r\n### version ");
	cli_print(c, UBLOAD_VERSION);
	cli_print(c, ", build date ");
	cli_print(c, UBLOAD_BUILD_DATE);
	cli_print(c, "\r\n" ESC_DEFAULT);

	return CLI_PRINT_BANNER_OK;
}


int32_t cli_print_key(struct cli *c, const uint8_t *key, uint32_t size) {
	if (u_assert(c != NULL) ||
	    u_assert(key != NULL) ||
	    u_assert(size > 0)) {
		return CLI_PRINT_KEY_FAILED;
	}

	cli_print(c, "[");
	for (uint32_t i = 0; i < size; i++) {
		char s[5];
		snprintf(s, sizeof(s), "%02x", key[i]);
		cli_print(c, s);
	}
	cli_print(c, "]");

	return CLI_PRINT_KEY_OK;
}


static uint8_t cli_hex_char_value(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
}


int32_t cli_parse_key(struct cli *c, const char *s, uint8_t *key, uint32_t size) {
	if (u_assert(c != NULL) ||
	    u_assert(s != NULL) ||
	    u_assert(key != NULL) ||
	    u_assert(size > 0)) {
		return CLI_PARSE_KEY_FAILED;
	}

	memset(key, 0, size);

	uint32_t i = 0;
	while (i < size && *s) {
		uint8_t byte = 0;
		if (*s) {
			byte = cli_hex_char_value(*s);
			s++;
		}
		if (*s) {
			byte = byte << 4 | cli_hex_char_value(*s);
			s++;
		}
		key[i] = byte;
		i++;
	}

	/* Check the key length. */
	if (*s || i != size) {
		return CLI_PARSE_KEY_FAILED;
	}

	return CLI_PARSE_KEY_OK;
}
