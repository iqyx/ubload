/*
 * F4 discovery board port-specific configuration
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
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#include "FreeRTOS.h"
#include "port.h"
#include "task.h"

#include "module_led.h"
#include "interface_led.h"
#include "s1d13700_locm3.h"
#include "interface_display.h"
#include "ui_320240.h"

uint32_t SystemCoreClock;

/* F4 discovery port module instances. */
struct module_led led1;
struct module_s1d13700_locm3 display1;
struct ui_320240 ui1;


int32_t port_mcu_init(void) {

	/* Initialize systick interrupt for FreeRTOS. */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(15999);
	systick_interrupt_enable();
	systick_counter_enable();

	/* System clock is now at 16MHz (without PLL). */
	SystemCoreClock = 16000000;

	/* Initialize all required clocks. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);
	rcc_periph_clock_enable(RCC_TIM3);

	/* Initialize interrupts. */
	nvic_enable_irq(NVIC_USART1_IRQ);
	nvic_set_priority(NVIC_USART1_IRQ, 6 * 16);


	#if PORT_SERIAL == true
		#if PORT_SERIAL_USART == USART1
			rcc_periph_clock_enable(RCC_USART1);
		#endif
		#if PORT_SERIAL_USART == USART2
			rcc_periph_clock_enable(RCC_USART2);
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

	#if PORT_LED_BASIC == true
		gpio_mode_setup(PORT_LED_BASIC_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 1 << PORT_LED_BASIC_PIN);
	#endif

	return 0;
}


int32_t port_early_init(void) {
	return PORT_EARLY_INIT_OK;
}


/* GPIO port configuration for the S1D13700 display driver interface. */
const struct module_s1d13700_locm3_port s1d13700_config = {
	PORT_DISPLAY_DATA_PORT,
	PORT_DISPLAY_DATA_PORT_MASK,
	PORT_DISPLAY_DATA_PORT_SHIFT,
	PORT_DISPLAY_CS_PORT,
	PORT_DISPLAY_CS_PIN,
	PORT_DISPLAY_RD_PORT,
	PORT_DISPLAY_RD_PIN,
	PORT_DISPLAY_WR_PORT,
	PORT_DISPLAY_WR_PIN,
	PORT_DISPLAY_RST_PORT,
	PORT_DISPLAY_RST_PIN,
	PORT_DISPLAY_A0_PORT,
	PORT_DISPLAY_A0_PIN
};


int32_t port_module_init(void) {

	#if PORT_LED_BASIC == true
		module_led_init(&led1, "led1");
		module_led_set_port(&led1, PORT_LED_BASIC_PORT, 1 << PORT_LED_BASIC_PIN);
		hal_interface_set_name(&(led1.iface.descriptor), "led1");
		interface_led_loop(&led1.iface, 0xff);
	#endif

	#if PORT_DISPLAY == true
		module_s1d13700_locm3_init(&display1, "display1", &s1d13700_config);
		hal_interface_set_name(&(display1.iface.descriptor), "display1");

		ui_320240_init(&ui1, &display1.iface);
	#endif

	return PORT_MODULE_INIT_OK;
}


/* Configure dedicated timer (tim3) for runtime task statistics. It should be later
 * redone to use one of the system monotonic clocks with interface_clock. */
void port_task_timer_init(void) {

	timer_reset(TIM3);
	/* The timer should run at 1MHz */
	timer_set_prescaler(TIM3, 1599);
	timer_continuous_mode(TIM3);
	timer_set_period(TIM3, UINT16_MAX);
	timer_enable_counter(TIM3);
}


uint32_t port_task_timer_get_value(void) {
	return timer_get_counter(TIM3);
}

void usart1_isr(void) {
	/* module_usart_interrupt_handler(&console); */
}

