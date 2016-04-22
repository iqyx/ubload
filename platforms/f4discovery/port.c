/**
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


uint32_t SystemCoreClock;

struct module_led led1;
struct module_s1d13700_locm3 display1;


qdlPos battery_indicator_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
char battery_indicator_text[10] = "62%";

/*
 * Simple graph indicator with a 0-100% text.
 */
qdlWidget battery_indicator = {
	.shape = QDL_GROUP,
	.position = {0, 0},
	.size = {50, 14},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {0, 0},
				.size = {50, 13},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_WHITE
				}
			},
			&(qdlWidget) {
				.shape = QDL_HISTOGRAM,
				.position = {29, 1},
				.size = {20, 10},
				.properties.histogram = {
					.data = battery_indicator_data,
					.data_size = 10,
					.bar_color = COLOR_BLACK,
					.bar_width = 2,
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {2, 1},
				.size = {20, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &minecraftia,
					.text = battery_indicator_text,
				},
			},
			NULL
		},
	}

};

/*
 * Vertical battery indicator, no text.
 */
qdlWidget battery_indicator2 = {
	.shape = QDL_GROUP,
	.position = {0, 0},
	.size = {16, 32},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {0, 4},
				.size = {16, 27},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_WHITE,
				}
			},
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {4, 0},
				.size = {8, 4},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_WHITE,
				}
			},
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {2, 13},
				.size = {12, 16},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_GRAY,
					.fill_color = COLOR_SILVER,
				}
			},
			NULL
		},
	}

};

/*
 * Horizontal battery indicator with 0-100% text below.
 */
qdlWidget battery_indicator3 = {
	.shape = QDL_GROUP,
	.position = {0, 3},
	.size = {32, 26},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {0, 0},
				.size = {27, 16},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_WHITE,
				}
			},
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {27, 4},
				.size = {4, 8},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_WHITE,
				}
			},
			&(qdlWidget) {
				.shape = QDL_RECT,
				.position = {2, 2},
				.size = {16, 12},
				.properties.rect = {
					.border_width = 1,
					.border_color = COLOR_BLACK,
					.fill_color = COLOR_BLACK,
				}
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 17},
				.size = {32, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &dejavu_sans_bold_10,
					.text = battery_indicator_text,
				},
			},
			NULL
		},
	}

};


char charging_status_text[20] = "Discharging 2.6W";
/*
 * Charging status indicator.
 */
qdlWidget charging_status = {
	.shape = QDL_GROUP,
	.position = {36, 0},
	.size = {96, 32},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 0},
				.size = {96, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &minecraftia,
					.text = "3.78V",
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 10},
				.size = {96, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &minecraftia,
					.text = "-1.2A",
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 20},
				.size = {96, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &dejavu_sans_bold_10,
					.text = "4.5W",
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {40, 0},
				.size = {64, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &dejavu_sans_bold_10,
					.text = "USB CHG",
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {40, 12},
				.size = {96, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &dejavu_sans_bold_10,
					.text = "1h 23m",
				},
			},
			NULL
		},
	}

};



qdlWidget scene = {
	.shape = QDL_GROUP,
	.size = {320, 240},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&battery_indicator3,
			&charging_status,
			NULL
		},
	},
};


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

	/* Status LEDs. */
	gpio_mode_setup(PORT_LED_BASIC_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 1 << PORT_LED_BASIC_PIN);

	return 0;
}


int32_t port_early_init(void) {
	return PORT_EARLY_INIT_OK;
}


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

	module_s1d13700_locm3_init(
		&display1,
		"display1",
		&s1d13700_config
	);
	hal_interface_set_name(&(display1.iface.descriptor), "display1");
	interface_display_set_screen(&display1.iface, &scene);

	if (PORT_LED_BASIC == true) {
		module_led_init(&led1, "led1");
		module_led_set_port(&led1, PORT_LED_BASIC_PORT, 1 << PORT_LED_BASIC_PIN);
		hal_interface_set_name(&(led1.iface.descriptor), "led1");
		interface_led_loop(&led1.iface, 0xff);
	}


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

