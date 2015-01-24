/**
 * qNode4 board port-specific configuration
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>

#include "config_port.h"


int32_t port_mcu_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	#if PORT_SERIAL == true
		#if PORT_SERIAL_USART == USART1
			rcc_periph_clock_enable(RCC_USART1);
		#endif
	#endif

	/* SPI flash memory. */
	#if PORT_SPI_FLASH == true
		#if PORT_SPI_FLASH_PORT == SPI2
			rcc_periph_clock_enable(RCC_SPI2);
		#endif
	#endif

	return 0;
}


int32_t port_gpio_init(void) {

	/* TODO: move led initialization here */
	#if PORT_SERIAL == true
		gpio_mode_setup(PORT_SERIAL_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, 1 << PORT_SERIAL_TX_PIN);
		gpio_mode_setup(PORT_SERIAL_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, 1 << PORT_SERIAL_RX_PIN);

		gpio_set_output_options(PORT_SERIAL_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, 1 << PORT_SERIAL_TX_PIN);
		gpio_set_output_options(PORT_SERIAL_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, 1 << PORT_SERIAL_RX_PIN);

		gpio_set_af(PORT_SERIAL_TX_PORT, PORT_SERIAL_AF, 1 << PORT_SERIAL_TX_PIN);
		gpio_set_af(PORT_SERIAL_RX_PORT, PORT_SERIAL_AF, 1 << PORT_SERIAL_RX_PIN);
	#endif

	#if PORT_SPI_FLASH == true
		gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
		gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13);
		gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO14);
		gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO15);

		gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO12);
		gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO13);
		gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO14);
		gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO15);

		gpio_set_af(GPIOB, 5, GPIO13);
		gpio_set_af(GPIOB, 5, GPIO14);
		gpio_set_af(GPIOB, 5, GPIO15);
		gpio_set(GPIOB, GPIO12);
	#endif

	return 0;
}
