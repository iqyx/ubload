/**
 * uBLoad command line interface - commands
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

#include "config.h"
#include "config_port.h"
#include "u_log.h"
#include "u_assert.h"
#include "cli.h"
#include "cli_cmd.h"

#include "fw_image.h"
#include "xmodem.h"
#include "sffs.h"
#include "pubkey_storage.h"


int32_t cli_print_help_command(struct cli *c, char *cmd, char *help) {
	if (c == NULL) {
		return CLI_PRINT_HELP_COMMAND_FAILED;
	}

	cli_print(c, ESC_BOLD);
	cli_print(c, cmd);
	cli_print(c, ESC_DEFAULT "\r\n");
	cli_print(c, help);
	cli_print(c, "\r\n");

	return CLI_PRINT_HELP_COMMAND_OK;
}


int32_t cli_cmd_help(struct cli *c) {
	if (c == NULL) {
		return CLI_CMD_HELP_FAILED;
	}

	cli_print(c, "No space left for this help :(\r\n");

	return CLI_CMD_HELP_OK;
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
				cli_print(c, ESC_COLOR_FG_GREEN "OK " ESC_DEFAULT);
				cli_print_key(c, pubkey_storage_slots[i].pubkey, PUBKEY_STORAGE_SLOT_SIZE);
			} else {
				cli_print(c, ESC_COLOR_FG_RED "invalid" ESC_DEFAULT);
			}

		} else if (slot_state == PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_LOCKED) {
			cli_print(c, ESC_COLOR_FG_RED "locked" ESC_DEFAULT);
		} else {
			cli_print(c, "empty");
		}
		cli_print(c, "\r\n");
	}

	return CLI_CMD_PUBKEY_PRINT_OK;
}


int32_t cli_cmd_pubkey_add(struct cli *c, const char *pubkey) {
	if (u_assert(c != NULL) ||
	    u_assert(pubkey != NULL)) {
		return CLI_CMD_PUBKEY_ADD_FAILED;
	}

	/* TODO: parse pubkey here */
	uint8_t key[PUBKEY_STORAGE_SLOT_SIZE];
	if (cli_parse_key(c, pubkey, key, sizeof(key)) != CLI_PARSE_KEY_OK) {
		cli_print(c, "Key parsing failed.\r\n");
		return CLI_CMD_PUBKEY_ADD_FAILED;
	}

	/* Find first empty slot. */
	uint32_t slot_num = UINT32_MAX;
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_COUNT; i++) {
		if (pubkey_storage_check_if_slot_empty(&(pubkey_storage_slots[i])) == PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY) {
			slot_num = i;
			break;
		}
	}
	if (UINT32_MAX == slot_num) {
		cli_print(c, "No empty slot found - new key cannot be added.\r\n");
		return CLI_CMD_PUBKEY_ADD_FAILED;
	} else {
		cli_print(c, "You are going to write a key ");
		cli_print_key(c, key, sizeof(key));
		char s[20];
		snprintf(s, sizeof(s), " to slot %u.\r\n", (unsigned int)(slot_num + 1));
		cli_print(c, s);

		if (cli_confirm(c) == CLI_CONFIRM_YES) {
			cli_print(c, "\r\n");
			/* TODO: add command to set salt */
			u_log(system_log, LOG_TYPE_INFO, "pubkey_storage: setting new key for slot %u", slot_num + 1);
			pubkey_storage_set_salt((uint8_t *)"aaa", 3);
			pubkey_storage_set_slot_key(&(pubkey_storage_slots[slot_num]), key, PUBKEY_STORAGE_SLOT_SIZE);
			return CLI_CMD_PUBKEY_ADD_OK;
		} else {
			cli_print(c, "\r\n");
			return CLI_CMD_PUBKEY_ADD_FAILED;
		}
	}

	return CLI_CMD_PUBKEY_ADD_OK;
}


int32_t cli_cmd_pubkey_lock(struct cli *c, uint32_t slot_num) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_PUBKEY_LOCK_FAILED;
	}

	if ((slot_num < 1 || slot_num > PUBKEY_STORAGE_SLOT_COUNT) && (slot_num != UINT32_MAX)) {
		cli_print(c, "Invalid slot number.\r\n");
		return CLI_CMD_PUBKEY_LOCK_FAILED;
	}

	if (UINT32_MAX == slot_num) {
		cli_print(c, "You yre going to lock all empty slots.\r\n");
		if (cli_confirm(c) == CLI_CONFIRM_YES) {
			cli_print(c, "\r\n");
			for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_COUNT; i++) {
				if (pubkey_storage_check_if_slot_empty(&(pubkey_storage_slots[i])) == PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY) {
					u_log(system_log, LOG_TYPE_INFO, "pubkey_storage: locking slot %u", i + 1);
					pubkey_storage_lock_slot(&(pubkey_storage_slots[i]));
				}
			}
			return CLI_CMD_PUBKEY_LOCK_OK;
		} else {
			cli_print(c, "\r\n");
			return CLI_CMD_PUBKEY_LOCK_FAILED;
		}

	} else {
		cli_print(c, "You yre going to lock slot ");
		char s[20];
		snprintf(s, sizeof(s), "%u.\r\n", (unsigned int)(slot_num));
		cli_print(c, s);
		if (cli_confirm(c) == CLI_CONFIRM_YES) {
			cli_print(c, "\r\n");
			/* Slots are indexed from 0 but displayed from 1. */
			u_log(system_log, LOG_TYPE_INFO, "pubkey_storage: locking slot %u", slot_num);
			pubkey_storage_lock_slot(&(pubkey_storage_slots[slot_num - 1]));
			return CLI_CMD_PUBKEY_LOCK_OK;
		} else {
			cli_print(c, "\r\n");
			return CLI_CMD_PUBKEY_LOCK_FAILED;
		}
	}

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


int32_t cli_cmd_fs_download(struct cli *c, char *file) {
	if (u_assert(c != NULL) ||
	    u_assert(file != NULL)) {
		return CLI_CMD_FS_DOWNLOAD_FAILED;
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

	return CLI_CMD_FS_DOWNLOAD_OK;
}


int32_t cli_cmd_fs_upload(struct cli *c, char *file) {
	(void)file;
	cli_print(c, "Unimplemented.\r\n");

	return CLI_CMD_FS_UPLOAD_OK;
}


int32_t cli_cmd_fs_delete(struct cli *c, char *file) {
	(void)file;

	cli_print(c, "Unimplemented.\r\n");
	return CLI_CMD_FS_DELETE_OK;
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


int32_t cli_cmd_dump_console(struct cli *c, uint32_t addr, uint32_t len) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_DUMP_CONSOLE_FAILED;
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

	return CLI_CMD_DUMP_CONSOLE_OK;
}


int32_t cli_cmd_dump_xmodem(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_DUMP_XMODEM_FAILED;
	}

	return CLI_CMD_DUMP_XMODEM_OK;
}


int32_t cli_cmd_dump_file(struct cli *c, const char *file) {
	if (u_assert(c != NULL && file != NULL)) {
		return CLI_CMD_DUMP_FILE_FAILED;
	}

	return CLI_CMD_DUMP_FILE_OK;
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


int32_t cli_cmd_log_print(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_LOG_PRINT_FAILED;
	}

	log_cbuffer_print(system_log);

	return CLI_CMD_LOG_PRINT_OK;
}


int32_t cli_cmd_fs_format(struct cli *c) {

	cli_print(c, "You are going to format the SFFS filesystem (all saved data will be lost).\r\n");
	if (cli_confirm(c) == CLI_CONFIRM_YES) {
		cli_print(c, "\r\n");
	} else {
		cli_print(c, "\r\n");
		return CLI_CMD_FS_FORMAT_FAILED;
	}

	/* Unmounting is not needed for sffs. */
	/* TODO: register sffs progress callback */
	u_log(system_log, LOG_TYPE_INFO, "sffs: creating new filesystem");
	if (sffs_format(&flash1) != SFFS_FORMAT_OK) {
		u_log(system_log, LOG_TYPE_ERROR, "sffs: formatting failed");
		return CLI_CMD_FS_FORMAT_FAILED;
	}
	u_log(system_log, LOG_TYPE_INFO, "sffs: new filesystem created.");

	/* TODO: call ubload_flash_init instead */
	sffs_init(&flash_fs);
	if (sffs_mount(&flash_fs, &flash1) != SFFS_MOUNT_OK) {
		u_log(system_log, LOG_TYPE_ERROR, "sffs: error while mounting sffs filesystem");
	}
	u_log(system_log, LOG_TYPE_INFO, "sffs: filesystem mounted successfully");

	return CLI_CMD_FS_FORMAT_OK;
}


int32_t cli_cmd_config_print_key(struct cli *c, const char *key) {
	if (u_assert(c != NULL && key != NULL)) {
		return CLI_CMD_CONFIG_PRINT_KEY_FAILED;
	}

	char s[50] = {0};

	if (!strcmp(key, "host")) {
		snprintf(s, sizeof(s), "'%s'", running_config.host);
	}

	if (s[0] != '\0') {
		cli_print(c, key);
		cli_print(c, " = ");
		cli_print(c, s);
		cli_print(c, "\r\n");
		return CLI_CMD_CONFIG_PRINT_KEY_OK;
	} else {
		cli_print(c, "Unknown configuration key '");
		cli_print(c, key);
		cli_print(c, "'\r\n");
		return CLI_CMD_CONFIG_PRINT_KEY_FAILED;
	}
}


int32_t cli_cmd_config_print_all(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_CONFIG_PRINT_ALL_FAILED;
	}

	cli_cmd_config_print_key(c, "host");

	return CLI_CMD_CONFIG_PRINT_ALL_OK;
}


int32_t cli_cmd_config_set(struct cli *c, const char *key, const char *value) {
	if (u_assert(c != NULL && key != NULL && value != NULL)) {
		return CLI_CMD_CONFIG_SET_FAILED;
	}

	if (!strcmp(key, "host")) {
		strlcpy(running_config.host, value, sizeof(running_config.host));
		return CLI_CMD_CONFIG_SET_OK;
	}

	return CLI_CMD_CONFIG_SET_FAILED;
}


int32_t cli_cmd_config_default(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_CONFIG_DEFAULT_FAILED;
	}

	/* Initialize running configuration using defaults. */
	memcpy(&running_config, &default_config, sizeof(running_config));
	u_log(system_log, LOG_TYPE_INFO, "config: default configuration loaded");

	return CLI_CMD_CONFIG_DEFAULT_OK;
}


int32_t cli_cmd_config_save(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_CONFIG_SAVE_FAILED;
	}

	struct sffs_file f;
	if (sffs_open(&flash_fs, &f, "ubload.cfg", SFFS_OVERWRITE) != SFFS_OPEN_OK) {
		return CLI_CMD_CONFIG_SAVE_FAILED;
	}
	if (sffs_write(&f, (uint8_t *)&running_config, sizeof(running_config)) != sizeof(running_config)) {
		u_log(system_log, LOG_TYPE_ERROR, "config: error saving the configuration");
		return CLI_CMD_CONFIG_SAVE_FAILED;
	}
	sffs_close(&f);
	u_log(system_log, LOG_TYPE_INFO, "config: running configuration saved");

	return CLI_CMD_CONFIG_SAVE_OK;
}


int32_t cli_cmd_config_load(struct cli *c) {
	if (u_assert(c != NULL)) {
		return CLI_CMD_CONFIG_LOAD_FAILED;
	}

	u_log(system_log, LOG_TYPE_INFO, "config: loading saved running configuration");
	struct sffs_file f;
	if (sffs_open(&flash_fs, &f, "ubload.cfg", SFFS_READ) != SFFS_OPEN_OK) {
		u_log(system_log, LOG_TYPE_ERROR, "config: cannot open saved configuration");
		return CLI_CMD_CONFIG_LOAD_FAILED;
	}
	if (sffs_read(&f, (uint8_t *)&running_config, sizeof(running_config)) != sizeof(running_config)) {
		u_log(system_log, LOG_TYPE_ERROR, "config: error reading saved configuration");
		return CLI_CMD_CONFIG_LOAD_FAILED;
	}
	sffs_close(&f);

	return CLI_CMD_CONFIG_LOAD_OK;
}


