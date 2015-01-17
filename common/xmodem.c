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

#include <stdint.h>
#include <stdlib.h>

#include <libopencm3/stm32/usart.h>

#include "config.h"
#include "xmodem.h"
#include "timer.h"


int32_t xmodem_init(struct xmodem *x, uint32_t console) {
	if (x == NULL) {
		return XMODEM_INIT_FAILED;
	}

	x->console = console;
	x->packet_timeout = XMODEM_DEFAULT_PACKET_TIMEOUT;
	x->retry_count = XMODEM_DEFAULT_RETRY_COUNT;

	return XMODEM_INIT_OK;
}


int32_t xmodem_free(struct xmodem *x) {
	if (x == NULL) {
		return XMODEM_FREE_FAILED;
	}

	/* Nothing to do here. */

	return XMODEM_FREE_OK;
}


int32_t xmodem_recv_packet(struct xmodem *x, uint8_t *data) {
	if (x == NULL || data == NULL) {
		return XMODEM_RECV_PACKET_FAILED;
	}

	uint16_t chr;

	/* Capture actual system time as a base for packet timeout. */
	uint32_t start_time;
	timer_timeout_start(&start_time);

	/* TODO: this part is meh. */
	/* Wait for SOH character */
	chr = 0;
	while (chr != XMODEM_SOH) {
		while (!usart_get_flag(x->console, USART_SR_RXNE)) {
			if (!timer_timeout_check(start_time, x->packet_timeout)) {
				return XMODEM_RECV_PACKET_TIMEOUT;
			}
		}
		chr = usart_recv(x->console);

		/* Break transfer if EOT is received instead of SOH. */
		if (chr == XMODEM_EOT) {
			return XMODEM_RECV_PACKET_EOT;
		}

		/* Break transfer if ESC is pressed. */
		if (chr == XMODEM_ESC) {
			return XMODEM_RECV_PACKET_CANCEL;
		}

	}

	/* Wait for packet number and inverse packet number. */
	while (!usart_get_flag(x->console, USART_SR_RXNE)) {
		if (!timer_timeout_check(start_time, x->packet_timeout)) {
			return XMODEM_RECV_PACKET_TIMEOUT;
		}
	}
	uint16_t pkt = usart_recv(x->console);

	while (!usart_get_flag(x->console, USART_SR_RXNE)) {
		if (!timer_timeout_check(start_time, x->packet_timeout)) {
			return XMODEM_RECV_PACKET_TIMEOUT;
		}
	}
	uint16_t pkti = usart_recv(x->console);

	/* Check the packet numbers. */
	if (pkt != x->pkt_expected || (uint8_t)(~pkti) != x->pkt_expected) {
		/* TODO: this packet is actually not that bad, just
		 * the ACK was missed and the packet was resent again. */
		return XMODEM_RECV_PACKET_BAD_PACKET;
	}

	/* Now read 128 bytes of packet data and compute checksum. */
	uint32_t checksum_computed = 0;
	for (uint32_t i = 0; i < 128; i++) {
		while (!usart_get_flag(x->console, USART_SR_RXNE)) {
			if (!timer_timeout_check(start_time, x->packet_timeout)) {
				return XMODEM_RECV_PACKET_TIMEOUT;
			}
		}
		data[i] = usart_recv(x->console);
		checksum_computed += data[i];
	}
	checksum_computed &= 0xff;

	/* Read packet checksum. */
	while (!usart_get_flag(x->console, USART_SR_RXNE)) {
		if (!timer_timeout_check(start_time, x->packet_timeout)) {
			return XMODEM_RECV_PACKET_TIMEOUT;
		}
	}
	uint8_t checksum = usart_recv(x->console);

	/* And finally verify packet checksum. */
	if (checksum != checksum_computed) {
		return XMODEM_RECV_PACKET_BAD_PACKET;
	}

	return XMODEM_RECV_PACKET_OK;
}


int32_t xmodem_set_recv_callback(struct xmodem *x, int32_t (*recv_cb)(uint8_t *data, uint32_t len, void *ctx), void *recv_cb_ctx) {
	if (x == NULL || recv_cb == NULL) {
		return XMODEM_SET_RECV_CALLBACK_FAILED;
	}

	x->recv_cb = recv_cb;
	x->recv_cb_ctx = recv_cb_ctx;

	return XMODEM_SET_RECV_CALLBACK_OK;
}


int32_t xmodem_recv(struct xmodem *x) {
	if (x == NULL) {
		return XMODEM_RECV_FAILED;
	}

	uint32_t retry = 0;
	x->pkt_expected = 1;

	while (1) {
		uint8_t data[128];
		int32_t res = xmodem_recv_packet(x, data);

		if (res == XMODEM_RECV_PACKET_FAILED ||
		    res == XMODEM_RECV_PACKET_TIMEOUT ||
		    res == XMODEM_RECV_PACKET_BAD_PACKET) {
			/* TODO: send NAK */
			retry++;

			if (retry >= x->retry_count) {
				return XMODEM_RECV_TIMEOUT;
			}

			usart_send_blocking(x->console, XMODEM_NAK);
		}

		if (res == XMODEM_RECV_PACKET_OK) {
			/* TODO: save packet data */
			if (x->recv_cb != NULL) {
				if (x->recv_cb(data, 128, x->recv_cb_ctx) != XMODEM_RECV_CB_OK) {
					/* Break the transfer if something went
					 * wrong or breaking the transfer was
					 * requested from inside of recv_cb. */
					usart_send_blocking(x->console, XMODEM_CAN);
					usart_send_blocking(x->console, XMODEM_CAN);

					/* TODO: really return EOT? */
					return XMODEM_RECV_PACKET_EOT;
				}
			}
			usart_send_blocking(x->console, XMODEM_ACK);
			retry = 0;
			x->pkt_expected++;
		}

		if (res == XMODEM_RECV_PACKET_EOT) {
			usart_send_blocking(x->console, XMODEM_ACK);
			return XMODEM_RECV_EOT;
		}

		if (res == XMODEM_RECV_PACKET_CANCEL) {
			usart_send_blocking(x->console, XMODEM_CAN);
			usart_send_blocking(x->console, XMODEM_CAN);
			return XMODEM_RECV_CANCEL;
		}
	}
}


int32_t xmodem_set_timeouts(struct xmodem *x, uint32_t packet_timeout, uint32_t retry_count) {
	if (x == NULL || packet_timeout == 0 || retry_count == 0) {
		return XMODEM_SET_TIMEOUTS_FAILED;
	}

	x->packet_timeout = packet_timeout;
	x->retry_count = retry_count;

	return XMODEM_SET_TIMEOUTS_OK;
}
