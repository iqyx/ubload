/**
 * uBLoad firmware image routines
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
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/iwdg.h>

#include "u_assert.h"
#include "u_log.h"
#include "config.h"
#include "config_port.h"
#include "fw_image.h"
#include "cli.h"

const char a[] = "0123456789abcdef";


int32_t fw_image_init(struct fw_image *fw, void *base) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_INIT_FAILED;
	}

	memset(fw, 0, sizeof(struct fw_image));
	fw->base = base;

	return FW_IMAGE_INIT_OK;
}


int32_t fw_image_jump(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_JUMP_FAILED;
	}

	register uint32_t msp __asm("msp");
	typedef void (*t_app_entry)(void);

	/* application vector table is positioned at the starting address +
	 * size of the header (1KB) */
	const uint32_t *vector_table = (uint32_t *)((uint8_t *)fw->base + 0x400);

	/* load application entry point to app_entry function pointer */
	t_app_entry app_entry = (t_app_entry)(vector_table[1]);

	/* set app stack pointer and jump to application */
	msp = vector_table[0];
	app_entry();
	(void)msp;

	/* never reached */
	return FW_IMAGE_JUMP_OK;
}


int32_t fw_image_reset(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_RESET_FAILED;
	}

	/* TODO: remove this magic. */
        *((unsigned long*)0xE000ED0C) = 0x05FA0004;
        while (1) {
		;
	}

	/* Unreachable. */
	return FW_IMAGE_RESET_OK;
}


int32_t fw_image_watchdog_enable(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_WATCHDOG_ENABLE_FAILED;
	}

	iwdg_set_period_ms(5000);
	iwdg_start();

	return FW_IMAGE_WATCHDOG_ENABLE_OK;
}

/* TODO: authenticate */
/* TODO: check firmware header + if it is valid according to configuration */
/* TODO: check firmware integrity */


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


int32_t fw_flash_dump(struct cli *c, uint32_t addr, uint32_t len) {

	for (uint32_t i = 0; i < len; i++) {
		/* Print line header */
		if ((i % 16) == 0) {
			char s[9];
			hex_to_string32(s, addr + i);
			cli_print(c, "0x");
			cli_print(c, s);
			cli_print(c, ": ");
		}

		/* TODO: print byte here */
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

	return FW_FLASH_DUMP_OK;
}


int32_t fw_flash_erase_sector(uint8_t sector) {

	flash_unlock();
	flash_erase_sector(sector, FLASH_PROGRAM_SIZE);
	flash_lock();

	return FW_FLASH_ERASE_SECTOR_OK;
}


int32_t fw_flash_program(uint32_t offset, uint8_t *data, uint32_t len) {

	flash_unlock();
	flash_program(FW_IMAGE_BASE + offset, data, len);

	return FW_FLASH_PROGRAM_OK;
}


