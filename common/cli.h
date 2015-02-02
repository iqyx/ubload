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

#include "lineedit.h"

#define CLI_MAX_ARGC 5

/* TODO: meh */
extern struct fw_image main_fw;
extern struct sffs flash_fs;
extern struct flash_dev flash1;

struct cli {
	uint32_t console;
	struct lineedit le;
};


int32_t cli_init(struct cli *c, uint32_t console);
#define CLI_INIT_OK 0
#define CLI_INIT_FAILED -1

int32_t cli_free(struct cli *c);
#define CLI_FREE_OK 0
#define CLI_FREE_FAILED -1

int32_t cli_wait_keypress(struct cli *c);
#define CLI_WAIT_KEYPRESS_ENTER 1
#define CLI_WAIT_KEYPRESS_SKIP 0
#define CLI_WAIT_KEYPRESS_FAILED -1

int32_t cli_run(struct cli *c);
#define CLI_RUN_TIMEOUT 2
#define CLI_RUN_RESET 1
#define CLI_RUN_BOOT 0
#define CLI_RUN_FAILED -1

int32_t cli_confirm(struct cli *c);
#define CLI_CONFIRM_YES 1
#define CLI_CONFIRM_NO 0
#define CLI_CONFIRM_FAILED -1

int32_t cli_progress_callback(uint32_t progress, uint32_t total, void *ctx);

int32_t cli_parse_command(struct cli *c, char *cmd, uint32_t *argc, char *argv[], uint32_t max_argc);
#define CLI_PARSE_COMMAND_OK 0
#define CLI_PARSE_COMMAND_FAILED -1

int32_t cli_execute(struct cli *c, uint32_t argc, char *argv[]);
#define CLI_EXECUTE_OK 0
#define CLI_EXECUTE_FAILED -1

int32_t cli_print(struct cli *c, const char *s);
#define CLI_PRINT_OK 0
#define CLI_PRINT_FAILED -1

int32_t cli_print_banner(struct cli *c);
#define CLI_PRINT_BANNER_OK 0
#define CLI_PRINT_BANNER_FAILED -1

int32_t cli_print_key(struct cli *c, const uint8_t *key, uint32_t size);
#define CLI_PRINT_KEY_OK 0
#define CLI_PRINT_KEY_FAILED -1

int32_t cli_parse_key(struct cli *c, const char *s, uint8_t *key, uint32_t size);
#define CLI_PARSE_KEY_OK 0
#define CLI_PARSE_KEY_FAILED -1


#endif

