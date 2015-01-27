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

#ifndef _PUBKEY_STORAGE_H_
#define _PUBKEY_STORAGE_H_

#define PUBKEY_STORAGE_SLOT_SIZE 32
#define PUBKEY_STORAGE_SLOT_FP_SIZE 4
#define PUBKEY_STORAGE_SLOT_HASH_SIZE 64
#define PUBKEY_STORAGE_SLOT_COUNT 4
#define PUBKEY_STORAGE_SALT_SIZE 32

/* TODO: hash salt */

struct pubkey_storage_slot {
	/**
	 * Public key is stored here. Size of the public key can be actually
	 * lower than PUBKEY_STORAGE_SLOT_SIZE.
	 */
	const uint8_t pubkey[PUBKEY_STORAGE_SLOT_SIZE];

	/**
	 * SHA512 hash of the public key. It is updated when the public key is
	 * overwritten. Public key is considered valid if its hash is also valid.
	 */
	const uint8_t pubkey_hash[PUBKEY_STORAGE_SLOT_HASH_SIZE];

	/**
	 * Public key fingerprints are used to look up required key when multiple
	 * slots are available and valid.
	 */
	const uint8_t pubkey_fp[PUBKEY_STORAGE_SLOT_FP_SIZE];
};

/**
 * All slots available for the bootloader. Slots are initialized as empty by default
 * and can be later disabled (to disallow adding more keys).
 */
extern const struct pubkey_storage_slot pubkey_storage_slots[PUBKEY_STORAGE_SLOT_COUNT];

/**
 * Public key hashes are salted with a value set at first boot.
 */
extern const uint8_t pubkey_storage_salt[PUBKEY_STORAGE_SALT_SIZE];


/**
 * @brief Sets a key for the selected slot.
 *
 * The slot is first checked if it is empty. Overwriting used slot is not allowed.
 * Key fingerprint and salted hash is then computed and saved in the slot.
 *
 * @param slot Pubkey slot to save the key to.
 * @param key A buffer containing public key data.
 * @param size Size of the public key in the @a key buffer.
 *
 * @return PUBKEY_STORAGE_SET_SLOT_KEY_OK if the key was successfully set,
 *         PUBKEY_STORAGE_SET_SLOT_KEY_USED if the selected slot is already used or
 *         PUBKEY_STORAGE_SET_SLOT_KEY_FAILED otherwise.
 */
int32_t pubkey_storage_set_slot_key(const struct pubkey_storage_slot *slot, const uint8_t *key, uint8_t size);
#define PUBKEY_STORAGE_SET_SLOT_KEY_OK 0
#define PUBKEY_STORAGE_SET_SLOT_KEY_FAILED -1
#define PUBKEY_STORAGE_SET_SLOT_KEY_USED -2

/**
 * @brief Check if the selected slot is empty and ready to be written.
 *
 * @param slot Pubkey slot to check.
 *
 * @return PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY if the slot is empty/erased,
 *         PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_LOCKED if the key is locked and unwritable,
 *         PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_USED if a key was previously saved here or
 *         PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_FAILED otherwise.
 */
int32_t pubkey_storage_check_if_slot_empty(const struct pubkey_storage_slot *slot);
#define PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_LOCKED 2
#define PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_EMPTY 1
#define PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_USED 0
#define PUBKEY_STORAGE_CHECK_IF_SLOT_EMPTY_FAILED -1

/**
 * @brief Verify if a suitable salt is available.
 *
 * @return PUBKEY_STORAGE_VERIFY_SALT_OK if a suitable salt is available or
 *         PUBKEY_STORAGE_VERIFY_SALT_FAILED otherwise.
 */
int32_t pubkey_storage_verify_salt(void);
#define PUBKEY_STORAGE_VERIFY_SALT_OK 0
#define PUBKEY_STORAGE_VERIFY_SALT_FAILED -1

/**
 * @brief Verify selected slot integrity.
 *
 * Key fingerprint and salted hash are both checked if they were generated
 * from the public key saved in the slot.
 *
 * @param slot Pubkey slot to verify.
 *
 * @return PUBKEY_STORAGE_VERIFY_SLOT_OK if the slot is considered valid or
 *         PUBKEY_STORAGE_VERIFY_SLOT_FAILED otherwise.
 */
int32_t pubkey_storage_verify_slot(const struct pubkey_storage_slot *slot);
#define PUBKEY_STORAGE_VERIFY_SLOT_OK 0
#define PUBKEY_STORAGE_VERIFY_SLOT_FAILED -1

/**
 * @brief Verify selected slot and retrieve its public key.
 *
 * @param slot Pubkey slot to verify and retrieve the key from.
 * @param key A buffer of at least @a size size where the key will be placed.
 * @param size Size of the key to retrieve, PUBKEY_STORAGE_SLOT_SIZE maximum.
 *
 * @return PUBKEY_STORAGE_GET_SLOT_KEY_OK if slot vas verified and the key
 *                                        was retrieved successfully or
 *         PUBKEY_STORAGE_GET_SLOT_KEY_FAILED otherwise.
 */
int32_t pubkey_storage_get_slot_key(struct pubkey_storage_slot *slot, uint8_t *key, uint8_t size);
#define PUBKEY_STORAGE_GET_SLOT_KEY_OK 0
#define PUBKEY_STORAGE_GET_SLOT_KEY_FAILED -1

/**
 * @brief Compare public key fingerprint saved in the slot with a
 *        fingerprint provided as an argument.
 *
 * @param slot Pubkey slot with fingerprint to compare.
 * @param fp A fingerprint of PUBKEY_STORAGE_SLOT_FP_SIZE size to compare.
 *
 * @return PUBKEY_STORAGE_COMPARE_FP_MATCH if both fingerprints match,
 *         PUBKEY_STORAGE_COMPARE_FP_NO_MATCH if the fingerprints do not match or
 *         PUBKEY_STORAGE_COMPARE_FP_FAILED otherwise.
 */
int32_t pubkey_storage_compare_fp(struct pubkey_storage_slot *slot, uint8_t *fp);
#define PUBKEY_STORAGE_COMPARE_FP_MATCH 1
#define PUBKEY_STORAGE_COMPARE_FP_NO_MATCH 0
#define PUBKEY_STORAGE_COMPARE_FP_FAILED -1

int32_t pubkey_storage_set_salt(const uint8_t *salt, uint32_t size);
#define PUBKEY_STORAGE_SET_SALT_OK 0
#define PUBKEY_STORAGE_SET_SALT_FAILED -1
#define PUBKEY_STORAGE_SET_SALT_HAVE_SOME -2

int32_t pubkey_storage_lock_slot(const struct pubkey_storage_slot *slot);
#define PUBKEY_STORAGE_LOCK_SLOT_OK 0
#define PUBKEY_STORAGE_LOCK_SLOT_FAILED -1


#endif
