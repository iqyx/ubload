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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "config.h"
#include "config_port.h"
#include "led_basic.h"
#include "fw_runner.h"
#include "timer.h"

#if PORT_LED_BASIC == true
struct led_basic led_stat;
#endif

struct fw_runner runner;




static int32_t mcu_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	return 0;
}


int main(void) {
	mcu_init();
	timer_init();

	#if PORT_LED_BASIC == true
		led_basic_init(&led_stat, PORT_LED_BASIC_PORT, PORT_LED_BASIC_PIN, PORT_LED_BASIC_INV);
	#endif

	fw_runner_init(&runner, (void *)FW_RUNNER_BASE);
	//~ fw_runner_jump(&runner);

	while (1) {
		#if PORT_LED_BASIC == true
			led_basic_timer(&led_stat);
		#endif

		timer_wait_ms(100);
	}

	return 0;
}
