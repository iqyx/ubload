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
#include "cli.h"
#include "timer.h"
#include "lineedit.h"
#include "fw_image.h"
#include "xmodem.h"
#include "u_log.h"
#include "sffs.h"


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


static int32_t cli_xmodem_recv_to_flash_cb(uint8_t *data, uint32_t len, uint32_t offset, void *ctx) {
	struct fw_image *fw = (struct fw_image *)ctx;
	fw_image_program(fw, offset, data, len);

	return XMODEM_RECV_CB_OK;
}


static int32_t cli_xmodem_recv_to_file_cb(uint8_t *data, uint32_t len, uint32_t offset, void *ctx) {
	(void)offset;

	struct sffs_file *f = (struct sffs_file *)ctx;

	/* We are ignoring the offset. */
	sffs_write(f, data, len);

	return XMODEM_RECV_CB_OK;
}


static int32_t cli_progress_callback(uint32_t progress, uint32_t total, void *ctx) {
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

	return FW_IMAGE_PROGRESS_CALLBACK_OK;
}


int32_t cli_execute(struct cli *c, char *cmd) {
	if (c == NULL || cmd == NULL) {
		return CLI_EXECUTE_FAILED;
	}

	if (!strcmp(cmd, "")) {
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "dump")) {
		fw_flash_dump(c, FW_IMAGE_BASE, 0x1000);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "help")) {
		cli_print_help(c);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "erase")) {
		fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)c);
		fw_image_erase(&main_fw);
		cli_print(c, "\r\n");
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "program xmodem")) {
		cli_print(c, "Go ahead and send your firmware using XMODEM... (press ESC to cancel)\r\n");

		struct xmodem x;
		xmodem_init(&x, c->console);
		xmodem_set_recv_callback(&x, cli_xmodem_recv_to_flash_cb, (void *)&main_fw);
		xmodem_recv(&x);

		/* Clear the terminal after xmodem transfer. */
		cli_print(c, "                   \r\n");
		char s[40];
		snprintf(s, sizeof(s), "%u bytes programmed.\r\n", (unsigned int)(x.bytes_transferred));
		cli_print(c, s);

		xmodem_free(&x);

		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "download")) {
		cli_print(c, "Go ahead and send your firmware using XMODEM... (press ESC to cancel)\r\n");

		struct sffs_file f;
		sffs_open_id(&flash_fs, &f, 1000, SFFS_OVERWRITE);

		struct xmodem x;
		xmodem_init(&x, c->console);
		xmodem_set_recv_callback(&x, cli_xmodem_recv_to_file_cb, (void *)&f);
		xmodem_recv(&x);
		cli_print(c, "\r\n");
		xmodem_free(&x);

		sffs_close(&f);

		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "program")) {

		struct sffs_file f;
		sffs_open_id(&flash_fs, &f, 1000, SFFS_READ);

		uint32_t size = 0;
		sffs_file_size(&flash_fs, 1000, &size);

		char s[80];
		snprintf(s, sizeof(s), "Flashing firmware %s, size %u bytes\r\n", "unknown", (unsigned int)(size));
		cli_print(c, s);

		cli_progress_callback(0, size, (void *)c);
		int32_t len = 0;
		uint32_t offset = 0;
		uint32_t update = 0;
		uint8_t buf[128];
		while ((len = sffs_read(&f, buf, sizeof(buf))) > 0) {
			fw_image_program(&main_fw, offset, buf, len);
			offset += len;
			update += len;

			if (update >= 1024) {
				cli_progress_callback(offset, size, (void *)c);
				update = 0;
			}
		}
		sffs_close(&f);
		cli_print(c, "\r\n");

		return CLI_EXECUTE_OK;
	}

	if (!strcmp(cmd, "verify")) {
		uint8_t hash[64];

		fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)c);
		fw_image_hash_compare(&main_fw, (uint8_t *)FW_IMAGE_BASE, 65536, hash);

		cli_print(c, "\r\n");
		return CLI_EXECUTE_OK;
	}

	cli_print(c, "Unknown command '");
	cli_print(c, cmd);
	cli_print(c, "'\r\n");

	return CLI_EXECUTE_FAILED;
}


int32_t cli_print(struct cli *c, char *s) {
	if (c == NULL || s == NULL) {
		return CLI_PRINT_FAILED;
	}

	cli_print_handler(s, (void *)c);

	return CLI_PRINT_OK;
}


int32_t cli_print_help_command(struct cli *c, char *cmd, char *help) {
	if (c == NULL) {
		return CLI_PRINT_HELP_COMMAND_FAILED;
	}

	lineedit_escape_print(&(c->le), ESC_BOLD, 0);
	cli_print(c, cmd);
	lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);
	cli_print(c, "\r\n");
	cli_print(c, help);
	cli_print(c, "\r\n");

	return CLI_PRINT_HELP_COMMAND_OK;
}


int32_t cli_print_help(struct cli *c) {
	if (c == NULL) {
		return CLI_PRINT_HELP_FAILED;
	}

	cli_print(c,
		"\r\nAvailable commands:\r\n"
		"([] are optional parameters, <> are obligatory parameters)\r\n\r\n"
	);

	cli_print_help_command(c,
		"help",
		"\tPrint this help."
	);
	cli_print_help_command(c,
		"reset",
		"\tReset/reboot the device with confirmation."
	);
	cli_print_help_command(c,
		"quit",
		"\tAlias for <reset>."
	);
	cli_print_help_command(c,
		"boot",
		"\tBoot to a previously loaded firmware image.\r\n"
		"\tNo firmware integrity check or authentication is performed."
	);
	cli_print_help_command(c,
		"load [name]",
		"\tLoad configuraton with name <name>. If no name is specified,\r\n"
		"\tdefault saved startup configuration is loaded."
	);
	cli_print_help_command(c,
		"save [name]",
		"\tSave current running configuration as a file named <name>,\r\n"
		"\tIf no name is given, configuration is saved as a startup configuration."
	);
	cli_print_help_command(c,
		"defaults",
		"\tLoad configuration defaults."
	);
	cli_print_help_command(c,
		"show [name]",
		"\tShow value of the selected configuration variable <name>.\r\n"
		"\tDisplay values of all variables if no name is given."
	);
	cli_print_help_command(c,
		"set <name> <value>",
		"\tAssign value <value> to a configuration variable <name>."
	);
	cli_print_help_command(c,
		"erase",
		"\tErase loaded firmware image. Bootloader is preserved."
	);
	cli_print_help_command(c,
		"dump <start> <length>",
		"\tDump <length> bytes of the loaded firmware image starting\r\n"
		"\tfrom offset <start>."
	);
	cli_print_help_command(c,
		"program <name>",
		"\tProgram firmware image named <name>. Program firmware downloaded\r\n"
		"\tdirectly over XMODEM if <xmodem> name is given."
	);
	cli_print_help_command(c,
		"verify [name]",
		"\tVerify firmware integrity. If no firmware <name> is given,\r\n"
		"\tcurrently loaded firmware is verified."
	);
	cli_print_help_command(c,
		"authenticate [name]",
		"\tAuthenticate selected firmware. If no firmware <name> is given,\r\n"
		"\tcurrently loaded firmware is authenticated."
	);
	cli_print_help_command(c,
		"download <name> [protocol]",
		"\tDownload firmware over serial port using protocol <protocol> and save\r\n"
		"\tit as a firmware named <name>. If no protocol is specified, XMODEM is used."
	);
	cli_print_help_command(c,
		"upload <name> [protocol]",
		"\tUpload firmware named <name> over serial port using protocol <protocol>.\r\n"
		"\tIf no protocol is specified, XMODEM is used."
	);
	cli_print_help_command(c,
		"list <config|firmware>",
		"\tList all available configuration or firmware files."
	);


	cli_print(c, "\r\n");


	return CLI_PRINT_HELP_OK;
}


int32_t cli_print_banner(struct cli *c) {
	if (c == NULL) {
		return CLI_PRINT_BANNER_FAILED;
	}

	lineedit_escape_print(&(c->le), ESC_BOLD, 0);
	lineedit_escape_print(&(c->le), ESC_COLOR, LINEEDIT_FG_COLOR_YELLOW);
	cli_print(c, "\r\n### ");
	cli_print(c, PORT_BANNER);
	cli_print(c, ", ");
	cli_print(c, PORT_NAME);
	cli_print(c, " platform\r\n### version ");
	cli_print(c, UBLOAD_VERSION);
	cli_print(c, ", build date ");
	cli_print(c, UBLOAD_BUILD_DATE);
	cli_print(c, "\r\n");
	lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);


	return CLI_PRINT_BANNER_OK;
}
