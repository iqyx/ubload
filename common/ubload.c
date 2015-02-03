/**
 * uBLoad bootloader entry point
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

#include "u_assert.h"
#include "u_log.h"
#include "config.h"
#include "config_port.h"
#include "led_basic.h"
#include "timer.h"
#include "cli.h"
#include "spi_flash.h"
#include "sffs.h"
#include "fw_image.h"

/* TODO: the whole file is meh. It needs to be rewritten. */


/*******************************************************************************
 * LED-related global variables and initialization.
 *
 * Global variable led_stat can be used to access main status LED.
 ******************************************************************************/
#if PORT_LED_BASIC == true
	struct led_basic led_stat;
#endif
static void ubload_led_init(void) {
	#if PORT_LED_BASIC == true
		led_basic_init(&led_stat, PORT_LED_BASIC_PORT, PORT_LED_BASIC_PIN, PORT_LED_BASIC_INV);
	#endif
}


/*******************************************************************************
 * Flash-related global variables and initialization.
 *
 * Global variable flash1 can be used to access the first SPI NOR flash
 * available to the system. No other memory devices are being initialized
 * nor needed.
 * Filesystem is mounted from this flash device during the initialization,
 * it can be accessed using flash_fs global variable.
 ******************************************************************************/
struct flash_dev flash1;
struct sffs flash_fs; /* TODO: cannot be static, CLI uses it */

static void ubload_flash_init(void) {
	flash_init(&flash1, PORT_SPI_FLASH_PORT, PORT_SPI_FLASH_CS_PORT, PORT_SPI_FLASH_CS_PIN);

	/* TODO: do this only if invalid flash data found. */
	/* sffs_format(&flash1); */
	sffs_init(&flash_fs);

	if (sffs_mount(&flash_fs, &flash1) == SFFS_MOUNT_OK) {
		u_log(system_log, LOG_TYPE_INFO, "sffs: filesystem mounted successfully");

		struct sffs_info info;
		if (sffs_get_info(&flash_fs, &info) == SFFS_GET_INFO_OK) {
			u_log(system_log, LOG_TYPE_INFO,
				"sffs: sectors t=%u e=%u u=%u f=%u d=%u o=%u, pages t=%u e=%u u=%u o=%u",
				info.sectors_total,
				info.sectors_erased,
				info.sectors_used,
				info.sectors_full,
				info.sectors_dirty,
				info.sectors_old,

				info.pages_total,
				info.pages_erased,
				info.pages_used,
				info.pages_old
			);
			u_log(system_log, LOG_TYPE_INFO,
				"sffs: space total %u bytes, used %u bytes, free %u bytes",
				info.space_total,
				info.space_used,
				info.space_total - info.space_used
			);
		}
	}
}


/*******************************************************************************
 * Main configuration initialization and loading
 *
 * There are two global variables defined in the config.h - default_config
 * (it contains default configuration values which are loaded when no other
 * valid configuration is found or when the user resets the running
 * configuration to defaults) and running_config, which can be used to access
 * actually valid configuration.
 ******************************************************************************/
static void ubload_config_init(void) {
	/* Initialize running configuration using defaults. */
	memcpy(&running_config, &default_config, sizeof(running_config));

	/* And try to load saved configuration */
	u_log(system_log, LOG_TYPE_INFO, "config: loading saved running configuration");
	struct sffs_file f;
	if (sffs_open(&flash_fs, &f, "ubload.cfg", SFFS_READ) != SFFS_OPEN_OK) {
		u_log(system_log, LOG_TYPE_ERROR, "config: cannot open saved configuration");
		return;
	}
	if (sffs_read(&f, (uint8_t *)&running_config, sizeof(running_config)) != sizeof(running_config)) {
		u_log(system_log, LOG_TYPE_ERROR, "config: error reading saved configuration");
	}
	sffs_close(&f);
}


/*******************************************************************************
 * Serial port initialization and global variables
 *
 * Global variable console can be used to access the first serial port
 * available in the system (usually used as a console serial port).
 ******************************************************************************/
#if PORT_SERIAL == true
	static uint32_t console;
#endif
static void ubload_serial_init(void) {
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
}


/*******************************************************************************
 * Command line interface - initialization and main cycle.
 * Error states are handled inside the ubload_cli function.
 *
 * Global variable console_cli can be used to access the only one global
 * instance of CLI available in the uBLoad (if it is initialized).
 ******************************************************************************/
static struct cli console_cli;
static void ubload_cli(void) {

	if (running_config.cli_enabled) {
		cli_init(&console_cli, PORT_SERIAL_USART);
		u_log_set_cli_print_handler(&console_cli);
		cli_print_banner(&console_cli);
		cli_print(&console_cli, "\r\n");
		log_cbuffer_print(system_log);

		int32_t wait_res = cli_wait_keypress(&console_cli);
		cli_print(&console_cli, "\r\n\r\n");
		if (wait_res == CLI_WAIT_KEYPRESS_ENTER) {

			cli_print(&console_cli, "uBLoad command line interface, type <help> to show available commands.\r\n\r\n");

			int32_t cli_res = cli_run(&console_cli);
			if (cli_res == CLI_RUN_BOOT) {
				/* Continue booting. */
				;
			} else {
				if (cli_res == CLI_RUN_TIMEOUT) {
					cli_print(&console_cli, "\r\nExiting CLI & doing reset due to inactivity...\r\n");
				}

				/* Reset on error, quit or reset command. */
				fw_image_reset(&main_fw);
			}
		} else {
			/* Continue booting on error, no keypress or skip keypress
			 * was detected. */
			;
		}
	}
}


/*******************************************************************************
 * Watchdog initialization
 ******************************************************************************/
static void ubload_watchdog_init(void) {
	if (running_config.watchdog_enabled) {
		if (running_config.cli_enabled) {
			u_log(system_log, LOG_TYPE_INFO, "Enabling watchdog");
		}
		/* TODO: make the watchdog interval configurable. */
		fw_image_watchdog_enable(&main_fw, 5000);
	}
}


/*******************************************************************************
 * This function performs actual device booting
 ******************************************************************************/
static void ubload_boot(void) __attribute__((noreturn));
static void ubload_boot(void) {
	if (running_config.cli_enabled) {
		u_log(system_log, LOG_TYPE_INFO, "Jumping to user code");
	}
	fw_image_jump(&main_fw);

	while (1) {
		;
	}
}


/*******************************************************************************
 * Firmware verification and authentication.
 ******************************************************************************/
#define UBLOAD_CHECK_FW_OK 0
#define UBLOAD_CHECK_FW_FAILED -1
static int32_t ubload_check_fw(void) {
	/* u_log(system_log, LOG_TYPE_WARN, "Wrong firmware loaded, performing fallback."); */

	/* Bypass this check for now. */
	return UBLOAD_CHECK_FW_OK;
}


/*******************************************************************************
 * Firmware verification and authentication.
 ******************************************************************************/
#define UBLOAD_AUTHENTICATE_OK 0
#define UBLOAD_AUTHENTICATE_FAILED -1
static int32_t ubload_authenticate(void) {

	if (running_config.cli_enabled) {
		fw_image_set_progress_callback(&main_fw, cli_progress_callback, (void *)&console_cli);
	}

	if (fw_image_verify(&main_fw) != FW_IMAGE_VERIFY_OK) {
		u_log(system_log, LOG_TYPE_CRIT, "Required firmware verification failed, requesting reset.");
		return UBLOAD_AUTHENTICATE_FAILED;
	}

	if (fw_image_authenticate(&main_fw) != FW_IMAGE_AUTHENTICATE_OK) {
		u_log(system_log, LOG_TYPE_CRIT, "Required firmware authentication failed, requesting reset.");
		return UBLOAD_AUTHENTICATE_FAILED;
	}
	return UBLOAD_AUTHENTICATE_OK;
}


struct fw_image main_fw;
int main(void) {
	/* Initialize circular log before any other things. */
	#if PORT_CLOG == true
		u_log_init();
	#endif

	port_mcu_init();
	port_gpio_init();
	timer_init();
	ubload_led_init();
	ubload_flash_init();
	ubload_config_init();

	fw_image_init(&main_fw, (void *)FW_IMAGE_BASE, FW_IMAGE_BASE_SECTOR, FW_IMAGE_SECTORS);

	ubload_serial_init();
	ubload_cli();

	if (ubload_check_fw() != UBLOAD_CHECK_FW_OK || ubload_authenticate() != UBLOAD_AUTHENTICATE_OK) {

		/* TODO: flash here if bad header is found or firmware integrity check
		 * failed or authentication failed */
		/* TODO: reboot here after firmware flashing */
		timer_wait_ms(2000);
		fw_image_reset(&main_fw);

	}

	ubload_watchdog_init();
	ubload_boot();

	/* Unreachable. Reset just in case anything goes wrong. */
	fw_image_reset(&main_fw);
	while (1) {
		;
	}
}
