/**
 * SPI Flash driver
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

#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "u_assert.h"
#include "u_log.h"
#include "spi_flash.h"
#include "timer.h"


int32_t flash_init(struct flash_dev *flash, uint32_t spi, uint32_t cs_port, uint8_t cs_pin) {
	if (u_assert(flash != NULL)) {
		return FLASH_INIT_FAILED;
	}

	flash->spi = spi;
	flash->cs_port = cs_port;
	flash->cs_pin = cs_pin;

	/* Setup SPI peripheral */
	spi_set_master_mode(flash->spi);
	spi_set_baudrate_prescaler(flash->spi, SPI_CR1_BR_FPCLK_DIV_2);
	spi_set_clock_polarity_0(flash->spi);
	spi_set_clock_phase_0(flash->spi);
	spi_set_full_duplex_mode(flash->spi);
	spi_set_unidirectional_mode(flash->spi); /* bidirectional but in 3-wire */
	spi_enable_software_slave_management(flash->spi);
	spi_send_msb_first(flash->spi);
	spi_set_nss_high(flash->spi);
	spi_enable(flash->spi);

	/* Try to communicate and detect flash memory. */
	struct flash_info info;
	if (flash_get_info(flash, &info) != FLASH_GET_INFO_OK) {
		/* Communication failed or unknown flash memory detected. */
		return FLASH_INIT_FAILED;
	}

	u_log(system_log, LOG_TYPE_INFO,
		"spi_flash: flash detected %s %s, size %u bytes",
		info.manufacturer,
		info.part,
		info.capacity
	);

	return FLASH_INIT_OK;
}


int32_t flash_free(struct flash_dev *flash) {
	if (u_assert(flash != NULL)) {
		return FLASH_FREE_FAILED;
	}

	/* Nothing to do here. */

	return FLASH_FREE_OK;
}


int32_t flash_get_id(struct flash_dev *flash, uint32_t *id) {
	if (u_assert(flash != NULL) ||
	    u_assert(id != NULL)) {
		return FLASH_GET_ID_FAILED;
	}

	uint32_t n = 0;
	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0x9f);
	n = (n << 8) | spi_xfer(flash->spi, 0x00);
	n = (n << 8) | spi_xfer(flash->spi, 0x00);
	n = (n << 8) | spi_xfer(flash->spi, 0x00);
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	*id = n;
	return FLASH_GET_ID_OK;
}


int32_t flash_write_enable(struct flash_dev *flash, bool ena) {
	if (u_assert(flash != NULL)) {
		return FLASH_WRITE_ENABLE_FAILED;
	}

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	if (ena) {
		spi_xfer(flash->spi, 0x06);
	} else {
		spi_xfer(flash->spi, 0x04);
	}
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	return FLASH_WRITE_ENABLE_OK;
}


int32_t flash_get_status(struct flash_dev *flash, uint8_t *status) {
	if (u_assert(flash != NULL)) {
		return FLASH_GET_STATUS_FAILED;
	}

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0x05);
	*status = spi_xfer(flash->spi, 0x00);
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	return FLASH_GET_STATUS_OK;
}


int32_t flash_wait_complete(struct flash_dev *flash) {
	if (u_assert(flash != NULL)) {
		return FLASH_WAIT_COMPLETE_FAILED;
	}

	/* TODO: timeout */
	uint8_t sr;
	do {
		flash_get_status(flash, &sr);
	} while (sr & 0x01);

	return FLASH_WAIT_COMPLETE_OK;
}


int32_t flash_get_info(struct flash_dev *flash, struct flash_info *info) {
	if (u_assert(flash != NULL) ||
	    u_assert(info != NULL)) {
		return FLASH_GET_INFO_FAILED;
	}

	uint32_t id;
	if (flash_get_id(flash, &id) != FLASH_GET_ID_OK) {
		return FLASH_GET_INFO_FAILED;
	}

	/* Spansion S25FL208K */
	if (id == 0x00014014) {
		info->capacity = 1024 * 1024;
		info->page_size = 256;
		info->sector_size = 4096;
		info->block_size = 65536;
		info->manufacturer = "Spansion";
		info->part = "S25FL208K";
		return FLASH_GET_INFO_OK;
	}

	return FLASH_GET_INFO_FAILED;
}


int32_t flash_chip_erase(struct flash_dev *flash) {
	(void)flash;

	/* unimplemented */

	return FLASH_CHIP_ERASE_FAILED;
}


int32_t flash_block_erase(struct flash_dev *flash, const uint32_t addr) {
	if (u_assert(flash != NULL)) {
		return FLASH_BLOCK_ERASE_FAILED;
	}

	u_log(system_log, LOG_TYPE_DEBUG, "spi_flash: erasing block at address 0x%08x", addr);

	flash_write_enable(flash, true);

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0xd8);
	spi_xfer(flash->spi, (addr >> 16) & 0xff);
	spi_xfer(flash->spi, (addr >> 8) & 0xff);
	spi_xfer(flash->spi, addr & 0xff);
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	flash_wait_complete(flash);
	flash_write_enable(flash, false);

	return FLASH_BLOCK_ERASE_OK;
}


int32_t flash_sector_erase(struct flash_dev *flash, const uint32_t addr) {
	if (u_assert(flash != NULL)) {
		return FLASH_SECTOR_ERASE_FAILED;
	}

	u_log(system_log, LOG_TYPE_DEBUG, "spi_flash: erasing sector at address 0x%08x", addr);

	flash_write_enable(flash, true);

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0x20);
	spi_xfer(flash->spi, (addr >> 16) & 0xff);
	spi_xfer(flash->spi, (addr >> 8) & 0xff);
	spi_xfer(flash->spi, addr & 0xff);
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	flash_wait_complete(flash);
	flash_write_enable(flash, false);

	return FLASH_SECTOR_ERASE_OK;
}


int32_t flash_page_write(struct flash_dev *flash, const uint32_t addr, const uint8_t *data, const uint32_t len) {
	if (u_assert(flash != NULL) ||
	    u_assert(data != NULL) ||
	    u_assert(len > 0) ||
	    u_assert(len <= 256)) {
		return FLASH_PAGE_WRITE_FAILED;
	}

	flash_write_enable(flash, true);

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0x02);
	spi_xfer(flash->spi, (addr >> 16) & 0xff);
	spi_xfer(flash->spi, (addr >> 8) & 0xff);
	spi_xfer(flash->spi, addr & 0xff);
	for (uint32_t i = 0; i < len; i++) {
		spi_xfer(flash->spi, data[i]);
	}
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	flash_wait_complete(flash);
	flash_write_enable(flash, false);

	return FLASH_PAGE_WRITE_OK;
}


int32_t flash_page_read(struct flash_dev *flash, const uint32_t addr, uint8_t *data, const uint32_t len) {
	if (u_assert(flash != NULL) ||
	    u_assert(data != NULL) ||
	    u_assert(len > 0) ||
	    u_assert(len <= 256)) {
		return FLASH_PAGE_READ_FAILED;
	}

	gpio_clear(flash->cs_port, 1 << flash->cs_pin);
	spi_xfer(flash->spi, 0x03);
	spi_xfer(flash->spi, (addr >> 16) & 0xff);
	spi_xfer(flash->spi, (addr >> 8) & 0xff);
	spi_xfer(flash->spi, addr & 0xff);
	for (uint32_t i = 0; i < len; i++) {
		data[i] = spi_xfer(flash->spi, 0x00);
	}
	gpio_set(flash->cs_port, 1 << flash->cs_pin);

	return FLASH_PAGE_READ_OK;
}
