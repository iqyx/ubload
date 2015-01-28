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
#include <string.h>

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/iwdg.h>

#include "u_assert.h"
#include "u_log.h"
#include "config.h"
#include "config_port.h"
#include "fw_image.h"
#include "cli.h"
#include "sha512.h"
#include "edsign.h"
#include "pubkey_storage.h"


int32_t fw_image_init(struct fw_image *fw, void *base, uint8_t base_sector, uint8_t sectors) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_INIT_FAILED;
	}

	memset(fw, 0, sizeof(struct fw_image));
	fw->base = base;
	fw->base_sector = base_sector;
	fw->sectors = sectors;

	return FW_IMAGE_INIT_OK;
}


int32_t fw_image_jump(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_JUMP_FAILED;
	}

	if (fw->parsed == false) {
		return FW_IMAGE_JUMP_FAILED;
	}

	register uint32_t msp __asm("msp");
	typedef void (*t_app_entry)(void);

	/* application vector table is positioned at the starting address +
	 * size of the header (1KB) */
	const uint32_t *vector_table = (uint32_t *)((uint8_t *)fw->base + fw->offset);

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

	fw_image_watchdog_enable(fw, 200);

        while (1) {
		;
	}

	/* Unreachable. */
	return FW_IMAGE_RESET_OK;
}


int32_t fw_image_watchdog_enable(struct fw_image *fw, uint32_t period) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_WATCHDOG_ENABLE_FAILED;
	}

	iwdg_set_period_ms(period);
	iwdg_start();

	return FW_IMAGE_WATCHDOG_ENABLE_OK;
}


int32_t fw_image_erase(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_ERASE_FAILED;
	}

	/* TODO: erase progress callback */

	if (fw->progress_callback != NULL) {
		fw->progress_callback(0, fw->sectors, fw->progress_callback_ctx);
	}
	flash_unlock();
	for (uint32_t i = fw->base_sector; i < (fw->base_sector + fw->sectors); i++) {
		flash_erase_sector(i, FW_IMAGE_PROGRAM_SPEED);

		/* Handle erase progress and abort. */
		if (fw->progress_callback != NULL) {
			if (fw->progress_callback(i - fw->base_sector + 1, fw->sectors, fw->progress_callback_ctx) == FW_IMAGE_PROGRESS_CALLBACK_CANCEL) {
				return FW_IMAGE_ERASE_FAILED;
			}
		}
	}
	flash_lock();

	fw_image_init(fw, fw->base, fw->base_sector, fw->sectors);

	return FW_IMAGE_ERASE_OK;
}


int32_t fw_image_program(struct fw_image *fw, uint32_t offset, uint8_t *data, uint32_t len) {
	if (u_assert(fw != NULL) ||
	    u_assert(data != NULL) ||
	    u_assert(len > 0)) {
		return FW_IMAGE_PROGRAM_FAILED;
	}

	flash_unlock();
	flash_program((uint32_t)fw->base + offset, data, len);
	flash_lock();

	fw_image_init(fw, fw->base, fw->base_sector, fw->sectors);

	return FW_IMAGE_PROGRAM_OK;
}


int32_t fw_image_set_progress_callback(
	struct fw_image *fw,
	int32_t (*progress_callback)(uint32_t progress, uint32_t total, void *ctx),
	void *ctx
) {
	if (u_assert(fw != NULL) ||
	    u_assert(progress_callback != NULL)) {
		return FW_IMAGE_SET_PROGRESS_CALLBACK_FAILED;
	}

	fw->progress_callback = progress_callback;
	fw->progress_callback_ctx = ctx;

	return FW_IMAGE_SET_PROGRESS_CALLBACK_OK;
}


int32_t fw_image_hash_compare(struct fw_image *fw, uint8_t *data, uint32_t len, uint8_t *hash) {
	if (u_assert(fw != NULL) ||
	    u_assert(hash != NULL)) {
		return FW_IMAGE_HASH_COMPARE_FAILED;
	}

	struct sha512_state ctx;

	sha512_init(&ctx);

	if (fw->progress_callback != NULL) {
		fw->progress_callback(0, len, fw->progress_callback_ctx);
	}

	uint32_t rem = len;
	uint32_t last_progress = 0;
	while (rem >= SHA512_BLOCK_SIZE) {
		sha512_block(&ctx, data);

		rem -= SHA512_BLOCK_SIZE;
		data += SHA512_BLOCK_SIZE;
		last_progress += SHA512_BLOCK_SIZE;

		if ((fw->progress_callback != NULL) && (last_progress > 4096)) {
			fw->progress_callback(len - rem, len, fw->progress_callback_ctx);
			last_progress = 0;
		}
	}
	sha512_final(&ctx, data, len);
	uint8_t computed_hash[SHA512_HASH_SIZE];
	sha512_get(&ctx, computed_hash, 0, SHA512_HASH_SIZE);
	if (fw->progress_callback != NULL) {
		fw->progress_callback(len, len, fw->progress_callback_ctx);
	}

	if (!memcmp(computed_hash, hash, sizeof(computed_hash))) {
		return FW_IMAGE_HASH_COMPARE_OK;
	}

	return FW_IMAGE_HASH_COMPARE_FAILED;
}


int32_t fw_image_parse_section(struct fw_image *fw, uint8_t **section_base, struct fw_image_section *section) {
	if (u_assert(fw != NULL) ||
	    u_assert(section != NULL)) {
		return FW_IMAGE_PARSE_SECTION_FAILED;
	}

	uint32_t magic = 0;
	magic = magic << 8 | *(*section_base);
	magic = magic << 8 | *(*section_base + 1);
	magic = magic << 8 | *(*section_base + 2);
	magic = magic << 8 | *(*section_base + 3);

	switch (magic) {
		case FW_IMAGE_SECTION_MAGIC_VERIFIED:
			section->type = FW_IMAGE_SECTION_TYPE_VERIFIED;
			break;
		case FW_IMAGE_SECTION_MAGIC_VERIFICATION:
			section->type = FW_IMAGE_SECTION_TYPE_VERIFICATION;
			break;
		case FW_IMAGE_SECTION_MAGIC_DUMMY:
			section->type = FW_IMAGE_SECTION_TYPE_DUMMY;
			break;
		case FW_IMAGE_SECTION_MAGIC_FIRMWARE:
			section->type = FW_IMAGE_SECTION_TYPE_FIRMWARE;
			break;
		case FW_IMAGE_SECTION_MAGIC_SHA512:
			section->type = FW_IMAGE_SECTION_TYPE_SHA512;
			break;
		case FW_IMAGE_SECTION_MAGIC_ED25519:
			section->type = FW_IMAGE_SECTION_TYPE_ED25519;
			break;
		case FW_IMAGE_SECTION_MAGIC_FP:
			section->type = FW_IMAGE_SECTION_TYPE_FP;
			break;
		default:
			section->type = FW_IMAGE_SECTION_TYPE_UNKNOWN;
			u_log(system_log, LOG_TYPE_WARN, "fw_image: unknown section magic 0x%08x", magic);
			break;
	}

	uint32_t len = 0;
	len = len << 8 | *(*section_base + 4);
	len = len << 8 | *(*section_base + 5);
	len = len << 8 | *(*section_base + 6);
	len = len << 8 | *(*section_base + 7);

	/* TODO: check if the whole section fits inside the flash space */
	section->len = len;
	section->data = *section_base + 8;

	/* Advance to the next section */
	*section_base += 8 + len;

	return FW_IMAGE_PARSE_SECTION_OK;
}


int32_t fw_image_parse(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_PARSE_FAILED;
	}

	bool parse_ok = true;
	u_log(system_log, LOG_TYPE_INFO, "fw_image: parsing firmware image...");

	uint8_t *section_base = fw->base;
	uint8_t *section_end = 0;

	/* Try to parse top level sections - verified and verification sections. */
	if ((fw_image_parse_section(fw, &section_base, &fw->verified_section) != FW_IMAGE_PARSE_SECTION_OK) ||
	    (fw_image_parse_section(fw, &section_base, &fw->verification_section) != FW_IMAGE_PARSE_SECTION_OK)) {
		parse_ok = false;
		goto end;
	}
	if ((fw->verified_section.type != FW_IMAGE_SECTION_TYPE_VERIFIED) ||
	    (fw->verification_section.type != FW_IMAGE_SECTION_TYPE_VERIFICATION)) {
		parse_ok = false;
		goto end;
	}

	/* Parse all subsections in the verified section. */
	fw->have_firmware = false;
	section_base = fw->verified_section.data;
	section_end = section_base + fw->verified_section.len;
	while (section_base < section_end) {
		struct fw_image_section subsection;
		if (fw_image_parse_section(fw, &section_base, &subsection) != FW_IMAGE_PARSE_SECTION_OK) {
			parse_ok = false;
			goto end;
		}
		switch (subsection.type) {
			case FW_IMAGE_SECTION_TYPE_DUMMY:
				break;
			case FW_IMAGE_SECTION_TYPE_FIRMWARE:
				fw->have_firmware = true;
				fw->offset = subsection.data - fw->base;
				u_log(system_log, LOG_TYPE_INFO, "fw_image: firmware vector table found at 0x%08x", subsection.data);
				break;
			default:
				/* Do nothing for unknown (but otherwise valid) sections. */
				break;
		}

	}

	/* Parse all subsections in the verification section. */
	fw->have_hash = false;
	section_base = fw->verification_section.data;
	section_end = section_base + fw->verification_section.len;
	while (section_base < section_end) {
		struct fw_image_section subsection;
		if (fw_image_parse_section(fw, &section_base, &subsection) != FW_IMAGE_PARSE_SECTION_OK) {
			parse_ok = false;
			goto end;
		}
		switch (subsection.type) {
			case FW_IMAGE_SECTION_TYPE_DUMMY:
				break;
			case FW_IMAGE_SECTION_TYPE_SHA512:
				/* SHA512 hash must be exactly 64 bytes long. */
				if (subsection.len == 64) {
					fw->have_hash = true;
					fw->hash = subsection.data;
					fw->hash_type = FW_IMAGE_SECTION_HASH_SHA512;
				}
				break;
			case FW_IMAGE_SECTION_TYPE_ED25519:
				/* Ed25519 signature must be exactly 64 bytes long,
				 * it is invalid itherwise. */
				if (subsection.len == 64) {
					fw->have_signature = true;
					fw->signature = subsection.data;
				}
				break;
			case FW_IMAGE_SECTION_TYPE_FP:
				/* Public key fingerprint must be at least 4 bytes long */
				if (subsection.len >= 4) {
					fw->have_pubkey_fp = true;
					fw->pubkey_fp = subsection.data;
					fw->pubkey_fp_len = subsection.len;
				}
				break;
			default:
				/* Do nothing for unknown (but otherwise valid) sections. */
				break;
		}
	}

end:
	if (parse_ok == true) {
		u_log(system_log, LOG_TYPE_INFO, "fw_image: firmware structure check & parsing OK");
		fw->parsed = true;
		return FW_IMAGE_PARSE_OK;
	} else {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: firmware structure check & parsing failed");
		fw->parsed = false;
		return FW_IMAGE_PARSE_FAILED;
	}
}


int32_t fw_image_verify(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_VERIFY_FAILED;
	}

	/* If the firmware is not parsed, parse it first. */
	if (fw->parsed == false) {
		if (fw_image_parse(fw) != FW_IMAGE_PARSE_OK) {
			return FW_IMAGE_VERIFY_FAILED;
		}
	}

	if (fw->have_hash == false) {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: verify: no known firmware hash available");
		fw->verified = false;
		return FW_IMAGE_VERIFY_FAILED;
	}

	u_log(system_log, LOG_TYPE_INFO, "fw_image: verifying firmware integrity...");
	if (fw_image_hash_compare(&main_fw, fw->verified_section.data, fw->verified_section.len, fw->hash) == FW_IMAGE_HASH_COMPARE_OK) {
		u_log(system_log, LOG_TYPE_INFO, "fw_image: firmware verification OK");
		fw->verified = true;
		return FW_IMAGE_VERIFY_OK;
	} else {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: firmware verification failed");
		fw->verified = false;
		return FW_IMAGE_VERIFY_FAILED;
	}
}


int32_t fw_image_authenticate(struct fw_image *fw) {
	if (u_assert(fw != NULL)) {
		return FW_IMAGE_AUTHENTICATE_FAILED;
	}

	/* If the firmware is not parsed, parse it first. */
	if (fw->parsed == false) {
		if (fw_image_parse(fw) != FW_IMAGE_PARSE_OK) {
			return FW_IMAGE_AUTHENTICATE_FAILED;
		}
	}

	/* If the firmware is not integrity checked and the hash is not
	 * computed yet, do it now. */
	if (fw->verified == false) {
		if (fw_image_verify(fw) != FW_IMAGE_VERIFY_OK) {
			return FW_IMAGE_AUTHENTICATE_FAILED;
		}

	}

	if (fw->have_signature == false) {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: authenticate: no valid firmware signature found");
		fw->authenticated = false;
		return FW_IMAGE_AUTHENTICATE_FAILED;
	}

	if (fw->have_pubkey_fp == false) {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: no public key fingerprint found, cannot match any key");
		fw->authenticated = false;
		return FW_IMAGE_AUTHENTICATE_FAILED;
	}

	u_assert(fw->signature != NULL);
	u_assert(fw->pubkey_fp != NULL);

	u_log(system_log, LOG_TYPE_INFO, "fw_image: authenticating loaded firmware...");

	/* Find suitable public key for authentication. */
	/* TODO: handle fingerprint conflict. */
	/* TODO: check if the key is valid for this type of signature. */
	uint8_t pubkey[PUBKEY_STORAGE_SLOT_SIZE];
	if (pubkey_storage_get_slot_key_by_fp(fw->pubkey_fp, pubkey, sizeof(pubkey)) != PUBKEY_STORAGE_GET_SLOT_KEY_BY_FP_OK) {
		u_log(system_log, LOG_TYPE_CRIT,
			"fw_image: no matching public key found for fingerprint 0x%02x%02x%02x%02x",
			fw->pubkey_fp[0],
			fw->pubkey_fp[1],
			fw->pubkey_fp[2],
			fw->pubkey_fp[3]
		);
		fw->authenticated = false;
		return FW_IMAGE_AUTHENTICATE_FAILED;
	}

	/* TODO: determine hash size by its type */
	if (edsign_verify(fw->signature, pubkey, fw->hash, 64)) {
		u_log(system_log, LOG_TYPE_INFO, "fw_image: firmware authentication OK");
		fw->authenticated = true;
		return FW_IMAGE_AUTHENTICATE_OK;
	} else {
		u_log(system_log, LOG_TYPE_CRIT, "fw_image: firmware authentication failed");
		fw->authenticated = false;
		return FW_IMAGE_AUTHENTICATE_FAILED;
	}
}
