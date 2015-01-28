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

#include <stdint.h>
#include <stdbool.h>

/**
 * Section magic numbers (tag) as they appear in the firmware conversion
 * tool (createfw.py). They were chosen randomly.
 */
#define FW_IMAGE_SECTION_MAGIC_VERIFIED 0x1eda84bc
#define FW_IMAGE_SECTION_MAGIC_VERIFICATION 0x6ef44bc0
#define FW_IMAGE_SECTION_MAGIC_DUMMY 0xba50911a
#define FW_IMAGE_SECTION_MAGIC_FIRMWARE 0x40b80c0f
#define FW_IMAGE_SECTION_MAGIC_SHA512 0xb6eb9721
#define FW_IMAGE_SECTION_MAGIC_ED25519 0x9d6b1a99
#define FW_IMAGE_SECTION_MAGIC_FP 0x5bf0aa39

/**
 * Type of hash digest available in the firmware image.
 */
enum fw_image_section_hash {
	FW_IMAGE_SECTION_HASH_SHA512,
};

/**
 * Types of firmware sections (top and subsections).
 */
enum fw_image_section_type {
	FW_IMAGE_SECTION_TYPE_UNKNOWN,
	FW_IMAGE_SECTION_TYPE_VERIFIED,
	FW_IMAGE_SECTION_TYPE_VERIFICATION,
	FW_IMAGE_SECTION_TYPE_DUMMY,
	FW_IMAGE_SECTION_TYPE_FIRMWARE,
	FW_IMAGE_SECTION_TYPE_SHA512,
	FW_IMAGE_SECTION_TYPE_ED25519,
	FW_IMAGE_SECTION_TYPE_FP,
};

/**
 * Firmware image section specification. Every section contains a header with
 * section type (determined by the section magic number) with length of data part
 * and the actual section data.
 */
struct fw_image_section {
	enum fw_image_section_type type;
	uint32_t len;
	uint8_t *data;
};

struct fw_image {
	/**
	 * Firmware image is loaded at selected base address. First section header
	 * starts here (verified section).
	 */
	uint8_t *base;

	/**
	 * These two represent the sector numbers which cover the whole usable
	 * flash area.
	 */
	uint8_t base_sector;
	uint8_t sectors;

	/**
	 * Have_firmware together with offset can be set during firmware image
	 * parsing if firmware section is found. Offset tell us how far is the
	 * firmware section data (= vector table) from the firmware image base.
	 */
	bool have_firmware;
	uint32_t offset;

	/**
	 * Progress callback is used for all actions on the firmware image which
	 * can take longer time (erase, program, verify, etc.). If set, it is
	 * called periodically during these actions. It can be used to abort
	 * the action by returning appropriate value.
	 */
	int32_t (*progress_callback)(uint32_t progress, uint32_t total, void *ctx);
	void *progress_callback_ctx;

	/**
	 * Set to true if the firmware image was parsed successfully.
	 */
	bool parsed;

	/**
	 * Set to true if the firmware image integrity was verified.
	 */
	bool verified;

	/**
	 * Set to true if the firmware image was authenticated.
	 */
	bool authenticated;

	/**
	 * Structures describint the two main top level sections.
	 */
	struct fw_image_section verified_section;
	struct fw_image_section verification_section;

	/**
	 * Have_hash is set to true if a known hash has been found during
	 * firmware parsing. Hash contains address to parsed hash (hash section
	 * data). Hash_type is set according to parsed hash section type.
	 */
	bool have_hash;
	uint8_t *hash;
	enum fw_image_section_hash hash_type;

	/**
	 * Set to true if valid signature section was found.
	 */
	bool have_signature;
	uint8_t *signature;

	/**
	 * If a public key fingerprint section is found, use this information
	 * for faster public key selection.
	 */
	bool have_pubkey_fp;
	uint8_t *pubkey_fp;
	uint32_t pubkey_fp_len;
};


extern const uint8_t *test_priv_key;
extern const uint8_t *test_pub_key;
extern const uint8_t *test_message;
extern const uint8_t *test_signature;

int32_t fw_image_init(struct fw_image *fw, void *base, uint8_t base_sector, uint8_t sectors);
#define FW_IMAGE_INIT_OK 0
#define FW_IMAGE_INIT_FAILED -1

int32_t fw_image_jump(struct fw_image *fw);
#define FW_IMAGE_JUMP_OK 0
#define FW_IMAGE_JUMP_FAILED -1

int32_t fw_image_reset(struct fw_image *fw);
#define FW_IMAGE_RESET_OK 0
#define FW_IMAGE_RESET_FAILED -1

int32_t fw_image_watchdog_enable(struct fw_image *fw, uint32_t period);
#define FW_IMAGE_WATCHDOG_ENABLE_OK 0
#define FW_IMAGE_WATCHDOG_ENABLE_FAILED -1

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

int32_t fw_image_hash_compare(struct fw_image *fw, uint8_t *data, uint32_t len, uint8_t *hash);
#define FW_IMAGE_HASH_COMPARE_OK 0
#define FW_IMAGE_HASH_COMPARE_FAILED -1

int32_t fw_image_parse_section(struct fw_image *fw, uint8_t **section_base, struct fw_image_section *section);
#define FW_IMAGE_PARSE_SECTION_OK 0
#define FW_IMAGE_PARSE_SECTION_FAILED -1

int32_t fw_image_parse(struct fw_image *fw);
#define FW_IMAGE_PARSE_OK 0
#define FW_IMAGE_PARSE_FAILED -1

int32_t fw_image_verify(struct fw_image *fw);
#define FW_IMAGE_VERIFY_OK 0
#define FW_IMAGE_VERIFY_FAILED -1

int32_t fw_image_authenticate(struct fw_image *fw);
#define FW_IMAGE_AUTHENTICATE_OK 0
#define FW_IMAGE_AUTHENTICATE_FAILED -1

#endif

