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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <libopencm3/stm32/usart.h>

#include "fw_flash.h"
#include "cli.h"

const char a[] = "0123456789abcdef";


int32_t hex_to_string32(char *s, uint32_t n) {

	for (uint32_t i = 0; i < 8; i++) {
		s[i] = a[(n >> ((7 - i) * 4)) & 0xf];
	}
	s[8] = '\0';

	return HEX_TO_STRING32_OK;
}


int32_t hex_to_string8(char *s, uint8_t n) {

	s[0] = a[n >> 4];
	s[1] = a[n & 0xf];
	s[2] = '\0';

	return HEX_TO_STRING8_OK;
}


int32_t fw_flash_dump(uint32_t addr, uint32_t len) {

	for (uint32_t i = 0; i < len; i++) {
		/* Print line header */
		if ((i % 16) == 0) {
			char s[9];
			hex_to_string32(s, addr + i);
			cli_print(&console_cli, "0x");
			cli_print(&console_cli, s);
			cli_print(&console_cli, ": ");
		}

		/* TODO: print byte here */
		uint8_t byte = *((uint8_t *)(addr + i));
		char bs[3];
		hex_to_string8(bs, byte);
		cli_print(&console_cli, bs);
		cli_print(&console_cli, " ");

		if ((i % 16) == 7) {
			cli_print(&console_cli, " ");
		}

		if ((i % 16) == 15) {
			cli_print(&console_cli, "\r\n");
		}


	}

	return FW_FLASH_DUMP_OK;
}

