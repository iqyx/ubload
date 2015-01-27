/**
 * uBLoad public key storage
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

#include <libopencm3/stm32/flash.h>

#include "u_assert.h"
#include "u_log.h"
#include "sha512.h"
#include "pubkey_storage.h"


struct pubkey_storage_slot pubkey_storage_slots[PUBKEY_STORAGE_SLOT_COUNT];
uint8_t pubkey_storage_salt[PUBKEY_STORAGE_SALT_SIZE];


int32_t pubkey_storage_set_slot_key(struct pubkey_storage_slot *slot, uint8_t *key, uint8_t size) {
	if (u_assert(slot != NULL) ||
	    u_assert(key != NULL) ||
	    u_assert(size > 0) ||
	    u_assert(size <= PUBKEY_STORAGE_SLOT_SIZE) ||
	    u_assert(PUBKEY_STORAGE_SLOT_SIZE + PUBKEY_STORAGE_SALT_SIZE < SHA512_BLOCK_SIZE)) {
		return PUBKEY_STORAGE_SET_SLOT_KEY_FAILED;
	}

	/* Check if the key is free first. No update attempt will be made if the slot is not empty. */
	if (pubkey_storage_check_if_slot_empty(slot) != PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY) {
		return PUBKEY_STORAGE_SET_SLOT_KEY_USED;
	}

	/* Check if we have some salt to use. */
	if (pubkey_storage_verify_salt() != PUBKEY_STORAGE_VERIFY_SALT_OK) {
		return PUBKEY_STORAGE_SET_SLOT_KEY_FAILED;
	}

	/* Prepare public key fingerprint. Hash only the pubkey part. */
	struct sha512_state s;
	uint8_t fp[PUBKEY_STORAGE_SLOT_FP_SIZE];
	sha512_init(&s);
	sha512_final(&s, key, size);
	sha512_get(&s, fp, 0, PUBKEY_STORAGE_SLOT_FP_SIZE);

	/* Prepare slot data with salt. Fill the remaining bytes with 0. */
	uint8_t data[PUBKEY_STORAGE_SLOT_SIZE + PUBKEY_STORAGE_SALT_SIZE];
	memset(data, 0, sizeof(data));
	memcpy(data, key, size);
	memcpy(data + PUBKEY_STORAGE_SLOT_SIZE, pubkey_storage_salt, PUBKEY_STORAGE_SALT_SIZE);

	/* Prepare public key hash. We are hashing with the salt now. */
	uint8_t hash[PUBKEY_STORAGE_SLOT_HASH_SIZE];
	sha512_init(&s);
	sha512_final(&s, data, sizeof(data));
	sha512_get(&s, hash, 0, PUBKEY_STORAGE_SLOT_HASH_SIZE);

	flash_unlock();
	/* Save the pubkey from the data variable - it is padded with 0x00. */
	flash_program((uint32_t)slot->pubkey, data, PUBKEY_STORAGE_SLOT_SIZE);
	/* Save the hash of the salted and padded public key. */
	flash_program((uint32_t)slot->pubkey_hash, hash, PUBKEY_STORAGE_SLOT_HASH_SIZE);
	/* And finally save the pubkey fingerprint for faster lookup. */
	flash_program((uint32_t)slot->pubkey_fp, fp, PUBKEY_STORAGE_SLOT_FP_SIZE);
	flash_lock();

	return PUBKEY_STORAGE_SET_SLOT_KEY_OK;
}


int32_t pubkey_storage_check_if_slot_empty(struct pubkey_storage_slot *slot) {
	if (u_assert(slot != NULL)) {
		return PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_FAILED;
	}

	/* Check all three fields in the slot. If any of the bytes is different from
	 * 0xff, result will also be different. */
	uint8_t val = 0xff;
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_SIZE; i++) {
		val &= slot->pubkey[i];
	}
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_HASH_SIZE; i++) {
		val &= slot->pubkey_hash[i];
	}
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SLOT_FP_SIZE; i++) {
		val &= slot->pubkey_fp[i];
	}

	if (0xff == val) {
		return PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY;
	} else {
		return PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_USED;
	}
}


int32_t pubkey_storage_verify_salt(void) {
	uint8_t val = 0xff;
	for (uint32_t i = 0; i < PUBKEY_STORAGE_SALT_SIZE; i++) {
		val &= pubkey_storage_salt[i];
	}

	if (0xff == val) {
		return PUBKEY_STORAGE_VERIFY_SALT_FAILED;
	} else {
		return PUBKEY_STORAGE_VERIFY_SALT_OK;
	}
}


int32_t pubkey_storage_verify_slot(struct pubkey_storage_slot *slot) {
	if (u_assert(slot != NULL)) {
		return PUBKEY_STORAGE_VERIFY_SLOT_FAILED;
	}

	/* First, compute key fingerprint and compare it. */
	struct sha512_state s;
	uint8_t fp[PUBKEY_STORAGE_SLOT_FP_SIZE];
	sha512_init(&s);
	/* TODO: this will fail if pubkey size is lower than PUBKEY_STORAGE_SLOT_SIZE */
	sha512_final(&s, slot->pubkey, PUBKEY_STORAGE_SLOT_SIZE);
	sha512_get(&s, fp, 0, PUBKEY_STORAGE_SLOT_FP_SIZE);

	/* Return error if the key fingerprint is not valid. */
	if (memcmp(fp, slot->pubkey_fp, PUBKEY_STORAGE_SLOT_FP_SIZE)) {
		return PUBKEY_STORAGE_VERIFY_SLOT_FAILED;
	}

	/* Prepare data for hash computation. */
	uint8_t data[PUBKEY_STORAGE_SLOT_SIZE + PUBKEY_STORAGE_SALT_SIZE];
	memcpy(data, slot->pubkey, PUBKEY_STORAGE_SLOT_SIZE);
	memcpy(data + PUBKEY_STORAGE_SLOT_SIZE, pubkey_storage_salt, PUBKEY_STORAGE_SALT_SIZE);

	uint8_t hash[PUBKEY_STORAGE_SLOT_HASH_SIZE];
	sha512_init(&s);
	sha512_final(&s, data, sizeof(data));
	sha512_get(&s, hash, 0, PUBKEY_STORAGE_SLOT_HASH_SIZE);

	/* And compare computed and stored hashes. */
	if (memcmp(hash, slot->pubkey_hash, PUBKEY_STORAGE_SLOT_HASH_SIZE)) {
		return PUBKEY_STORAGE_VERIFY_SLOT_FAILED;
	}

	return PUBKEY_STORAGE_VERIFY_SLOT_OK;
}


int32_t pubkey_storage_get_slot_key(struct pubkey_storage_slot *slot, uint8_t *key, uint8_t size) {
	if (u_assert(slot != NULL) ||
	    u_assert(key != NULL) ||
	    u_assert(size <= PUBKEY_STORAGE_SLOT_SIZE)) {
		return PUBKEY_STORAGE_GET_SLOT_KEY_FAILED;
	}

	/* First check if the slot is used. */
	if (pubkey_storage_check_if_slot_empty(slot) != PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_USED) {
		return PUBKEY_STORAGE_GET_SLOT_KEY_FAILED;
	}

	/* And verify if the slot is still valid. */
	if (pubkey_storage_verify_slot(slot) != PUBKEY_STORAGE_VERIFY_SLOT_OK) {
		return PUBKEY_STORAGE_GET_SLOT_KEY_FAILED;
	}

	memcpy(key, slot->pubkey, size);

	return PUBKEY_STORAGE_GET_SLOT_KEY_OK;
}

