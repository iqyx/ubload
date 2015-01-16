/**
 * uBLoad bootloader entry point, qNode4 board port
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
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include "config.h"
#include "config_port.h"
#include "led_basic.h"
#include "fw_runner.h"
#include "timer.h"
#include "cli.h"

#if PORT_LED_BASIC == true
struct led_basic led_stat;
#endif

#if PORT_SERIAL == true
uint32_t console;
#endif

struct fw_runner runner;
struct cli console_cli;


int u_assert_func(const char *a, const char *f, int n) {
	(void)a;
	(void)f;
	(void)n;
	return 1;
}

static int32_t mcu_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	#if PORT_SERIAL == true
		#if PORT_SERIAL_USART == USART1
			rcc_periph_clock_enable(RCC_USART1);
		#endif
	#endif

	return 0;
}


static int32_t gpio_init(void) {

	#if PORT_SERIAL == true
		gpio_mode_setup(PORT_SERIAL_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, 1 << PORT_SERIAL_TX_PIN);
		gpio_mode_setup(PORT_SERIAL_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, 1 << PORT_SERIAL_RX_PIN);

		gpio_set_output_options(PORT_SERIAL_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, 1 << PORT_SERIAL_TX_PIN);
		gpio_set_output_options(PORT_SERIAL_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, 1 << PORT_SERIAL_RX_PIN);

		gpio_set_af(PORT_SERIAL_TX_PORT, PORT_SERIAL_AF, 1 << PORT_SERIAL_TX_PIN);
		gpio_set_af(PORT_SERIAL_RX_PORT, PORT_SERIAL_AF, 1 << PORT_SERIAL_RX_PIN);
	#endif

	return 0;
}


int main(void) {
	mcu_init();
	timer_init();
	gpio_init();
	fw_runner_init(&runner, (void *)FW_RUNNER_BASE);

	/* Initialize running configuration using defaults. */
	memcpy(&running_config, &default_config, sizeof(running_config));

	#if PORT_LED_BASIC == true
		led_basic_init(&led_stat, PORT_LED_BASIC_PORT, PORT_LED_BASIC_PIN, PORT_LED_BASIC_INV);
	#endif

	/* Configure console serial port if it is enabled. Disable CLI otherwise. */
	#if PORT_SERIAL == true
		console = PORT_SERIAL_USART;
		usart_set_baudrate(console, running_config.serial_speed);
		usart_set_mode(console, USART_MODE_TX_RX);
		usart_set_databits(console, 8);
		usart_set_stopbits(console, USART_STOPBITS_1);
		usart_set_parity(console, USART_PARITY_NONE);
		usart_set_flow_control(console, USART_FLOWCONTROL_NONE);
		usart_enable(console);

		/* TODO: print banner here. */
	#else
		running_config.cli_enabled = false;
	#endif

	if (running_config.cli_enabled) {
		cli_init(&console_cli, PORT_SERIAL_USART);

		int32_t wait_res = cli_wait_keypress(&console_cli);
		cli_print(&console_cli, "\r\n\r\n");
		if (wait_res == CLI_WAIT_KEYPRESS_ENTER) {

			cli_print_help(&console_cli);

			int32_t cli_res = cli_run(&console_cli);
			if (cli_res == CLI_RUN_BOOT) {
				/* Continue booting. */
				;
			} else {
				if (cli_res == CLI_RUN_TIMEOUT) {
					cli_print(&console_cli, "\r\nExiting CLI & doing reset due to inactivity...\r\n");
				}

				/* Reset on error, quit or reset command. */
				fw_runner_reset(&runner);
			}
		} else {
			/* Continue booting on error, no keypress or skip keypress
			 * was detected. */
			;
		}
	}

	/* TODO: check firmware header here */
	/* TODO: check firmware integrity here */
	/* TODO: authenticate firmware here */

	/* TODO: flash here if bad header is found or firmware integrity check
	 * failed or authentication failed */

	/* TODO: reboot here after firmware flashing */

	/* Boot here. */
	fw_runner_jump(&runner);

	while (1) {
		#if PORT_LED_BASIC == true
			led_basic_timer(&led_stat);
		#endif

		timer_wait_ms(100);
	}

	return 0;
}
