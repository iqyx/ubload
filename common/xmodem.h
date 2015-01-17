/**
 * uBLoad xmodem protocol support
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

#ifndef _XMODEM_H_
#define _XMODEM_H_

/**
 * XMODEM control character defines (according to ascii table).
 */
#define XMODEM_SOH 0x01
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_ESC 0x1b

#define XMODEM_DEFAULT_PACKET_TIMEOUT 1000
#define XMODEM_DEFAULT_RETRY_COUNT 10


struct xmodem {
	/**
	 * libopencm3 usart device used for XMODEM communication.
	 */
	uint32_t console;

	/**
	 * Time in milliseconds the xmodem receiver is allowed to wait
	 * for a single packet reception.
	 */
	uint32_t packet_timeout;

	/**
	 * Number of retries allowed when waiting for next packet. NAK character
	 * is sent back to the transmitter after each retry.
	 */
	uint32_t retry_count;

	/**
	 * Expected sequence number of the following data packet.
	 */
	uint8_t pkt_expected;

	/**
	 * Callback function called when a packet is received.
	 */
	int32_t (*recv_cb)(uint8_t *data, uint32_t len, void *ctx);
	/**
	 * Context of the callback function. It will be passed as the last
	 * argument when calling the callback function.
	 */
	void *recv_cb_ctx;
};



/**
 * @brief Initialize XMODEM context.
 *
 * This function must be called on a context before calling any other
 * XMODEM function.
 *
 * @param x Xmodem context to initialize
 * @param console libopencm3 usart device to use
 *
 * @return XMODEM_INIT_OK on success or
 *         XMODEM_INIT_FAILED otherwise.
 */
int32_t xmodem_init(struct xmodem *x, uint32_t console);
#define XMODEM_INIT_OK 0
#define XMODEM_INIT_FAILED -1

/**
 * @brief Free specified XMODEM context.
 *
 * @param x Xmodem context to free.
 *
 * @return XMODEM_FREE_OK on success or
 *         XMODEM_FREE_FAILED otherwise.
 */
int32_t xmodem_free(struct xmodem *x);
#define XMODEM_FREE_OK 0
#define XMODEM_FREE_FAILED -1

/**
 * @brief Receive single packet.
 *
 * Function waits for a paket header marker and then receives the whole
 * packet. If reception doesn't occur within specified time interval or any
 * of the required checks fails, appropriate error code is returned.
 *
 * @param x Xmodem context
 * @param data Pointer to data buffer with at least 128 bytes free for
 *             received data.
 *
 * @return XMODEM_RECV_PACKET_OK if a correct packet was successfully received,
 *         XMODEM_RECV_PACKET_TIMEOUT if single packet reception timeout elapsed,
 *         XMODEM_RECV_PACKET_BAD_PACKET if there was sequence number or
 *                                       checksum mismatch,
 *         XMODEM_RECV_PACKET_EOT if end of transfer was received,
 *         XMODEM_RECV_PACKET_CANCEL if the transfer was cancelled by the user
 *                                   pressing ESC key or
 *         XMODEM_RECV_PACKET_FAILED otherwise.
 */
int32_t xmodem_recv_packet(struct xmodem *x, uint8_t *data);
#define XMODEM_RECV_PACKET_OK 0
#define XMODEM_RECV_PACKET_FAILED -1
#define XMODEM_RECV_PACKET_TIMEOUT -2
#define XMODEM_RECV_PACKET_BAD_PACKET -3
#define XMODEM_RECV_PACKET_EOT -4
#define XMODEM_RECV_PACKET_CANCEL -5

/**
 * @brief Set callback used to process received data packets.
 *
 * Callback may be used to handle received data. It is allowed to block if the
 * processing takes longer time (transmit side timeouts must be considered).
 * Callback can abort the transfer by returning XMODEM_RECV_CB_TERMINATE.
 *
 * @param x Xmodem context.
 * @param recv_cb Pointer to callback function.
 *
 * @return XMODEM_SET_RECV_CALLBACK_OK on success or
 *         XMODEM_SET_RECV_CALLBACK_FAILED otherwise.
 */
int32_t xmodem_set_recv_callback(struct xmodem *x, int32_t (*recv_cb)(uint8_t *data, uint32_t len, void *ctx), void *recv_cb_ctx);
#define XMODEM_SET_RECV_CALLBACK_OK 0
#define XMODEM_SET_RECV_CALLBACK_FAILED -1
#define XMODEM_RECV_CB_OK 0
#define XMODEM_RECV_CB_FAILED -1
#define XMODEM_RECV_CB_TERMINATE -2

/**
 * @brief Start xmodem receive transfer.
 *
 * Xmodem reception is started by transmitting NAK characters over the serial
 * port. A receive callback must be set beforehand to allow the data to be
 * processed.
 *
 * @param x Xmodem context.
 *
 * @return XMODEM_RECV_EOT if a file was correctly received,
 *         XMODEM_RECV_TIMEOUT if a timeout occured during file reception or
 *         XMODEM_RECV_FAILED otherwise.
 */
int32_t xmodem_recv(struct xmodem *x);
#define XMODEM_RECV_EOT 0
#define XMODEM_RECV_FAILED -1
#define XMODEM_RECV_TIMEOUT -2
#define XMODEM_RECV_CANCEL -3

/**
 * @brief Set packet reception timeout and retry count for current transmission.
 *
 * @param x Xmodem context to set parameters on
 * @param packet_timeout A nonzero timeout in milliseconds
 * @param retry_count Nonzero number of NAK characters allowed to be sent.
 *
 * @return XMODEM_SET_TIMEOUTS_OK on success or
 *         XMODEM_SET_TIMEOUTS_FAILED otherwise.
 */
int32_t xmodem_set_timeouts(struct xmodem *x, uint32_t packet_timeout, uint32_t retry_count);
#define XMODEM_SET_TIMEOUTS_OK 0
#define XMODEM_SET_TIMEOUTS_FAILED -1

#endif


