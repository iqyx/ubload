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

#ifndef _CLI_CMD_H_
#define _CLI_CMD_H_






int32_t cli_print_help_command(struct cli *c, char *cmd, char *help);
#define CLI_PRINT_HELP_COMMAND_OK 0
#define CLI_PRINT_HELP_COMMAND_FAILED -1

int32_t cli_cmd_help(struct cli *c);
#define CLI_CMD_HELP_OK 0
#define CLI_CMD_HELP_FAILED -1

int32_t cli_cmd_pubkey_print(struct cli *c);
#define CLI_CMD_PUBKEY_PRINT_OK 0
#define CLI_CMD_PUBKEY_PRINT_FAILED -1

int32_t cli_cmd_pubkey_add(struct cli *c, const char *pubkey);
#define CLI_CMD_PUBKEY_ADD_OK 0
#define CLI_CMD_PUBKEY_ADD_FAILED -1

int32_t cli_cmd_pubkey_lock(struct cli *c, uint32_t slot_num);
#define CLI_CMD_PUBKEY_LOCK_OK 0
#define CLI_CMD_PUBKEY_LOCK_FAILED -1

int32_t cli_cmd_program_xmodem(struct cli *c);
#define CLI_CMD_PROGRAM_XMODEM_OK 0
#define CLI_CMD_PROGRAM_XMODEM_FAILED -1

int32_t cli_cmd_program_file(struct cli *c, char *file);
#define CLI_CMD_PROGRAM_FILE_OK 0
#define CLI_CMD_PROGRAM_FILE_FAILED -1

int32_t cli_cmd_download(struct cli *c, char *file);
#define CLI_CMD_DOWNLOAD_OK 0
#define CLI_CMD_DOWNLOAD_FAILED -1

int32_t cli_cmd_erase(struct cli *c);
#define CLI_CMD_ERASE_OK 0
#define CLI_CMD_ERASE_FAILED -1

int32_t cli_cmd_dump(struct cli *c, uint32_t addr, uint32_t len);
#define CLI_CMD_DUMP_OK 0
#define CLI_CMD_DUMP_FAILED -1

int32_t cli_cmd_verify_flash(struct cli *c);
#define CLI_CMD_VERIFY_FLASH_OK 0
#define CLI_CMD_VERIFY_FLASH_FAILED -1

int32_t cli_cmd_authenticate_flash(struct cli *c);
#define CLI_CMD_AUTHENTICATE_FLASH_OK 0
#define CLI_CMD_AUTHENTICATE_FLASH_FAILED -1

int32_t cli_cmd_log_print(struct cli *c);
#define CLI_CMD_LOG_PRINT_OK 0
#define CLI_CMD_LOG_PRINT_FAILED -1

int32_t cli_cmd_fs_format(struct cli *c);
#define CLI_CMD_FS_FORMAT_OK 0
#define CLI_CMD_FS_FORMAT_FAILED -1


#endif

