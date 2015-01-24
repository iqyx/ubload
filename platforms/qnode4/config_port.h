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

#ifndef _CONFIG_PORT_H_
#define _CONFIG_PORT_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include "version.h"

#define PORT_NAME                  "qNode4"
#define PORT_BANNER                "uBLoad (umeshFw bootloader)"

/* Enable basic status/diagnostic LED functionality. */
#define PORT_LED_BASIC             true
#define PORT_LED_BASIC_PORT        GPIOA
#define PORT_LED_BASIC_PIN         3
#define PORT_LED_BASIC_INV         true

/* Advanced LED diagnostics. If multicolor leds are used, define each color
 * as a separate LED. */
#define PORT_LED_ADVANCED          true
#define PORT_LED_ADVANCED_COUNT    3
#define PORT_LED_ADVANCED_PORTS    {GPIOA, GPIOA, GPIOA}
#define PORT_LED_ADVANCED_PINS     {6, 7, 8}
#define PORT_LED_ADVANCED_INV      {true, true, true}

/* Serial port settings */
#define PORT_SERIAL                true
#define PORT_SERIAL_USART          USART1
#define PORT_SERIAL_TX_PORT        GPIOB
#define PORT_SERIAL_TX_PIN         6
#define PORT_SERIAL_RX_PORT        GPIOB
#define PORT_SERIAL_RX_PIN         7
#define PORT_SERIAL_AF             GPIO_AF7

/* Firmware runner configuration */
#define FW_RUNNER_BASE             0x08008000

/* Internal flash configuration */
#define FLASH_SECTORS              6
#define FLASH_FW_FIRST_SECTOR      2
#define FLASH_PROGRAM_SIZE         3

/* Circular log configuration */
#define PORT_CLOG                  true
#define PORT_CLOG_BASE             0x20000000
#define PORT_CLOG_SIZE             0x800

/* External SPI fash configuration */
#define PORT_SPI_FLASH             true




int32_t port_mcu_init(void);
#define PORT_MCU_INIT_OK 0
#define PORT_MCU_INIT_FAILED -1

int32_t port_gpio_init(void);
#define PORT_GPIO_INIT_OK 0
#define PORT_GPIO_INIT_FAILED -1

#endif


