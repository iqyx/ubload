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

#ifndef _FW_FLASH_H_
#define _FW_FLASH_H_


struct fw_image {
	void *base;
	uint8_t base_sector;
	uint8_t sectors;

	int32_t (*progress_callback)(uint32_t progress, uint32_t total, void *ctx);
	void *progress_callback_ctx;

};

int32_t fw_image_init(struct fw_image *fw, void *base, uint8_t base_sector, uint8_t sectors);
#define FW_IMAGE_INIT_OK 0
#define FW_IMAGE_INIT_FAILED -1

int32_t fw_image_jump(struct fw_image *fw);
#define FW_IMAGE_JUMP_OK 0
#define FW_IMAGE_JUMP_FAILED -1

int32_t fw_image_reset(struct fw_image *fw);
#define FW_IMAGE_RESET_OK 0
#define FW_IMAGE_RESET_FAILED -1

int32_t fw_image_watchdog_enable(struct fw_image *fw);
#define FW_IMAGE_WATCHDOG_ENABLE_OK 0
#define FW_IMAGE_WATCHDOG_ENABLE_FAILED -1

int32_t hex_to_string32(char *s, uint32_t n);
#define HEX_TO_STRING32_OK 0
#define HEX_TO_STRING32_FAILED -1

int32_t hex_to_string8(char *s, uint8_t n);
#define HEX_TO_STRING8_OK 0
#define HEX_TO_STRING8_FAILED -1

int32_t fw_flash_dump(struct cli *c, uint32_t addr, uint32_t len);
#define FW_FLASH_DUMP_OK 0
#define FW_FLASH_DUMP_FAILED -1

int32_t fw_image_erase(struct fw_image *fw);
#define FW_IMAGE_ERASE_OK 0
#define FW_IMAGE_ERASE_FAILED -1

/**
 * @brief Program firmware image page.
 *
 * Overwrite itself with new data in @a data buffer with length @a len placed
 * @a offset bytes from the beginning. Firmware image must be erased first.
 *
 * @param fw A firmware image to overwrite.
 * @param offset Offset of the new page to be written.
 * @param data Buffer with data.
 * @param len Length of @a data buffer.
 *
 * @return FW_IMAGE_PROGRAM_OK if the page was written successfully or
 *         FW_IMAGE_PROGRAM_FAILED otherwise.
 */
int32_t fw_image_program(struct fw_image *fw, uint32_t offset, uint8_t *data, uint32_t len);
#define FW_IMAGE_PROGRAM_OK 0
#define FW_IMAGE_PROGRAM_FAILED -1

int32_t fw_image_set_progress_callback(
	struct fw_image *fw,
	int32_t (*progress_callback)(uint32_t progress, uint32_t total, void *ctx),
	void *ctx
);
#define FW_IMAGE_SET_PROGRESS_CALLBACK_OK 0
#define FW_IMAGE_SET_PROGRESS_CALLBACK_FAILED -1
#define FW_IMAGE_PROGRESS_CALLBACK_OK 0
#define FW_IMAGE_PROGRESS_CALLBACK_FAILED -1
#define FW_IMAGE_PROGRESS_CALLBACK_CANCEL -2

#endif

