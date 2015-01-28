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
#include "timer.h"
#include "lineedit.h"
#include "fw_image.h"
#include "xmodem.h"
#include "sffs.h"
#include "pubkey_storage.h"


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
		if (argc < 3) {
			cli_print(c, "Missing argument(s).\r\n");
		}
		if (argc == 3) {
			uint32_t origin = atoi(argv[1]);
			uint32_t length = atoi(argv[2]);

			cli_cmd_dump(c, FW_IMAGE_BASE + origin, length);
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

	if (!strcmp(argv[0], "download")) {

		if (argc == 1) {
			cli_print(c, "Required argument is missing.\r\n");
		}
		if (argc == 2) {
			cli_cmd_download(c, argv[1]);
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

	if (!strcmp(argv[0], "pubkey print")) {
		cli_cmd_pubkey_print(c);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "pubkey add")) {
		cli_cmd_pubkey_add(c);
		return CLI_EXECUTE_OK;
	}

	if (!strcmp(argv[0], "pubkey lock")) {
		cli_cmd_pubkey_lock(c);
		return CLI_EXECUTE_OK;
	}

	cli_print(c, "Unknown command '");
	cli_print(c, argv[0]);
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


int32_t cli_cmd_help(struct cli *c) {
	if (c == NULL) {
		return CLI_CMD_HELP_FAILED;
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


	return CLI_CMD_HELP_OK;
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


int32_t cli_cmd_pubkey_print(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_PUBKEY_PRINT_FAILED;
	}

	/* Iterate over all slots. */
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_COUNT; i++) {
		char s[20];
		snprintf(s, sizeof(s), "Slot %u: ", (unsigned int)(i + 1));
		cli_print(c, s);

		int32_t slot_state = pubkey_storage_check_if_slot_empty(&(pubkey_storage_slots[i]));
		if (slot_state == PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_USED) {
			if (pubkey_storage_verify_slot(&(pubkey_storage_slots[i])) == PUBKEY_STORAGE_VERIFY_SLOT_OK) {
				lineedit_escape_print(&(c->le), ESC_COLOR, LINEEDIT_FG_COLOR_GREEN);
				cli_print(c, "OK ");
				lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);
				cli_print_key(c, pubkey_storage_slots[i].pubkey, PUBKEY_STORAGE_SLOT_SIZE);
			} else {
				lineedit_escape_print(&(c->le), ESC_COLOR, LINEEDIT_FG_COLOR_RED);
				cli_print(c, "invalid");
				lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);
			}

		} else if (slot_state == PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_LOCKED) {
			lineedit_escape_print(&(c->le), ESC_COLOR, LINEEDIT_FG_COLOR_RED);
			cli_print(c, "locked");
			lineedit_escape_print(&(c->le), ESC_DEFAULT, 0);
		} else {
			cli_print(c, "empty");
		}
		cli_print(c, "\r\n");
	}

	return CLI_CMD_PUBKEY_PRINT_OK;
}


int32_t cli_cmd_pubkey_add(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_PUBKEY_ADD_FAILED;
	}

	pubkey_storage_set_salt((uint8_t *)"aaa", 3);
	pubkey_storage_set_slot_key(&(pubkey_storage_slots[0]), (uint8_t *)"aaaaaaaabbbbbbbbccccccccdddddddd", 32);

	return CLI_CMD_PUBKEY_ADD_OK;
}


int32_t cli_cmd_pubkey_lock(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_PUBKEY_LOCK_FAILED;
	}

	pubkey_storage_lock_slot(&(pubkey_storage_slots[0]));

	return CLI_CMD_PUBKEY_LOCK_OK;
}


static int32_t cli_xmodem_recv_to_flash_cb(uint8_t *data, uint32_t len, uint32_t offset, void *ctx) {
	struct fw_image *fw = (struct fw_image *)ctx;
	fw_image_program(fw, offset, data, len);

	return XMODEM_RECV_CB_OK;
}


int32_t cli_cmd_program_xmodem(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_PROGRAM_XMODEM_FAILED;
	}

	cli_print(c, "Go ahead and send your firmware using XMODEM... (press ESC to cancel)\r\n");

	struct xmodem x;
	xmodem_init(&x, c->console);
	xmodem_set_recv_callback(&x, cli_xmodem_recv_to_flash_cb, (void *)&main_fw);
	int32_t res = xmodem_recv(&x);

	if (res == XMODEM_RECV_EOT) {
		/* Clear the terminal after xmodem transfer. */
		cli_print(c, "                   \r\n");
		char s[40];
		snprintf(s, sizeof(s), "%u bytes programmed.\r\n", (unsigned int)(x.bytes_transferred));
		cli_print(c, s);
	}
	if (res == XMODEM_RECV_CANCEL) {
		cli_print(c, "XMODEM transfer cancelled.\r\n");
	}
	if (res == XMODEM_RECV_TIMEOUT) {
		cli_print(c, "XMODEM transfer timeout.\r\n");
	}
	xmodem_free(&x);

	return CLI_CMD_PROGRAM_XMODEM_OK;
}


static int32_t cli_xmodem_recv_to_file_cb(uint8_t *data, uint32_t len, uint32_t offset, void *ctx) {
	(void)offset;

	struct sffs_file *f = (struct sffs_file *)ctx;

	/* We are ignoring the offset. */
	sffs_write(f, data, len);

	return XMODEM_RECV_CB_OK;
}


/* TODO: convert file to file_id. */
int32_t cli_cmd_program_file(struct cli *c, char *file) {
	if (u_assert(c != NULL) ||
	    u_assert(file != NULL)) {
		return CLI_CMD_PROGRAM_FILE_FAILED;
	}

	struct sffs_file f;
	sffs_open_id(&flash_fs, &f, 1000, SFFS_READ);

	uint32_t size = 0;
	sffs_file_size(&flash_fs, 1000, &size);

	char s[80];
	snprintf(s, sizeof(s), "Flashing firmware %s, size %u bytes\r\n", file, (unsigned int)(size));
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

	return CLI_CMD_PROGRAM_FILE_OK;
}


int32_t cli_cmd_download(struct cli *c, char *file) {
	if (u_assert(c != NULL) ||
	    u_assert(file != NULL)) {
		return CLI_CMD_PROGRAM_FILE_FAILED;
	}

	cli_print(c, "Go ahead and send your firmware using XMODEM... (press ESC to cancel)\r\n");

	struct sffs_file f;
	sffs_open_id(&flash_fs, &f, 1000, SFFS_OVERWRITE);

	struct xmodem x;
	xmodem_init(&x, c->console);
	xmodem_set_recv_callback(&x, cli_xmodem_recv_to_file_cb, (void *)&f);
	int32_t res = xmodem_recv(&x);

	if (res == XMODEM_RECV_EOT) {
		/* Clear the terminal after xmodem transfer. */
		cli_print(c, "                   \r\n");
		char s[40];
		snprintf(s, sizeof(s), "%u bytes downloaded.\r\n", (unsigned int)(x.bytes_transferred));
		cli_print(c, s);
	}
	if (res == XMODEM_RECV_CANCEL) {
		cli_print(c, "XMODEM transfer cancelled.\r\n");
	}
	if (res == XMODEM_RECV_TIMEOUT) {
		cli_print(c, "XMODEM transfer timeout.\r\n");
	}
	xmodem_free(&x);
	sffs_close(&f);

	return CLI_CMD_DOWNLOAD_OK;
}


int32_t cli_cmd_erase(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_ERASE_FAILED;
	}

	fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)c);
	fw_image_erase(&main_fw);
	cli_print(c, "\r\n");

	return CLI_CMD_ERASE_OK;
}


static void hex_to_string32(char *s, uint32_t n) {
	const char a[] = "0123456789abcdef";
	for (uint32_t i = 0; i < 8; i++) {
		s[i] = a[(n >> ((7 - i) * 4)) & 0xf];
	}
	s[8] = '\0';
}


static void hex_to_string8(char *s, uint8_t n) {
	const char a[] = "0123456789abcdef";
	s[0] = a[n >> 4];
	s[1] = a[n & 0xf];
	s[2] = '\0';
}


int32_t cli_cmd_dump(struct cli *c, uint32_t addr, uint32_t len) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_DUMP_FAILED;
	}

	for (uint32_t i = 0; i < len; i++) {
		/* Print line header */
		if ((i % 16) == 0) {
			char s[9];
			hex_to_string32(s, addr + i);
			cli_print(c, "0x");
			cli_print(c, s);
			cli_print(c, ": ");
		}

		uint8_t byte = *((uint8_t *)(addr + i));
		char bs[3];
		hex_to_string8(bs, byte);
		cli_print(c, bs);
		cli_print(c, " ");

		if ((i % 16) == 7) {
			cli_print(c, " ");
		}
		if ((i % 16) == 15) {
			cli_print(c, "\r\n");
		}
	}

	return CLI_CMD_DUMP_OK;
}


int32_t cli_cmd_verify_flash(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_VERIFY_FLASH_FAILED;
	}

	fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)c);
	fw_image_verify(&main_fw);

	return CLI_CMD_VERIFY_FLASH_OK;
}


int32_t cli_cmd_authenticate_flash(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_AUTHENTICATE_FLASH_FAILED;
	}

	fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)c);
	fw_image_authenticate(&main_fw);

	return CLI_CMD_AUTHENTICATE_FLASH_OK;
}


