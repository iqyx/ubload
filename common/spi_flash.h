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

#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

#include <stdint.h>
#include <stdbool.h>

struct flash_dev {

	uint32_t spi;
	uint32_t cs_port;
	uint8_t cs_pin;
};


struct flash_info {
	uint32_t capacity;
	uint32_t page_size;
	uint32_t sector_size;
	uint32_t block_size;
	char *manufacturer;
	char *part;
};


int32_t flash_init(struct flash_dev *flash, uint32_t spi, uint32_t cs_port, uint8_t cs_pin);
#define FLASH_INIT_OK 0
#define FLASH_INIT_FAILED -1

int32_t flash_free(struct flash_dev *flash);
#define FLASH_FREE_OK 0
#define FLASH_FREE_FAILED -1

int32_t flash_get_id(struct flash_dev *flash, uint32_t *id);
#define FLASH_GET_ID_OK 0
#define FLASH_GET_ID_FAILED -1

int32_t flash_write_enable(struct flash_dev *flash, bool ena);
#define FLASH_WRITE_ENABLE_OK 0
#define FLASH_WRITE_ENABLE_FAILED -1

int32_t flash_get_status(struct flash_dev *flash, uint8_t *status);
#define FLASH_GET_STATUS_OK 0
#define FLASH_GET_STATUS_FAILED -1

int32_t flash_wait_complete(struct flash_dev *flash);
#define FLASH_WAIT_COMPLETE_OK 0
#define FLASH_WAIT_COMPLETE_FAILED -1

int32_t flash_get_info(struct flash_dev *flash, struct flash_info *info);
#define FLASH_GET_INFO_OK 0
#define FLASH_GET_INFO_FAILED -1

int32_t flash_chip_erase(struct flash_dev *flash);
#define FLASH_CHIP_ERASE_OK 0
#define FLASH_CHIP_ERASE_FAILED -1

int32_t flash_block_erase(struct flash_dev *flash, const uint32_t addr);
#define FLASH_BLOCK_ERASE_OK 0
#define FLASH_BLOCK_ERASE_FAILED -1

int32_t flash_sector_erase(struct flash_dev *flash, const uint32_t addr);
#define FLASH_SECTOR_ERASE_OK 0
#define FLASH_SECTOR_ERASE_FAILED -1

int32_t flash_page_write(struct flash_dev *flash, const uint32_t addr, const uint8_t *data, const uint32_t len);
#define FLASH_PAGE_WRITE_OK 0
#define FLASH_PAGE_WRITE_FAILED -1

int32_t flash_page_read(struct flash_dev *flash, const uint32_t addr, uint8_t *data, const uint32_t len);
#define FLASH_PAGE_READ_OK 0
#define FLASH_PAGE_READ_FAILED -1




#endif



