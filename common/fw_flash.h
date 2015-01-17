/**
 * uBLoad firmware flash routines
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

#ifndef _FW_FLASH_H_
#define _FW_FLASH_H_

/* TODO: meh */
extern struct cli console_cli;

int32_t hex_to_string32(char *s, uint32_t n);
#define HEX_TO_STRING32_OK 0
#define HEX_TO_STRING32_FAILED -1

int32_t hex_to_string8(char *s, uint8_t n);
#define HEX_TO_STRING8_OK 0
#define HEX_TO_STRING8_FAILED -1

int32_t fw_flash_dump(uint32_t addr, uint32_t len);
#define FW_FLASH_DUMP_OK 0
#define FW_FLASH_DUMP_FAILED -1

int32_t fw_flash_erase_sector(uint8_t sector);
#define FW_FLASH_ERASE_SECTOR_OK 0
#define FW_FLASH_ERASE_SECTOR_FAILED -1

int32_t fw_flash_program(uint32_t offset, uint8_t *data, uint32_t len);
#define FW_FLASH_PROGRAM_OK 0
#define FW_FLASH_PROGRAM_FAILED -1

#endif

