/*
 * uBLoad user interface for 320x240 pixel displays
 *
 * Copyright (C) 2016, Marek Koza, qyx@krtko.org
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

/**
 * @todo This is just a prototype of a graphical user interface suitable for 320x240 pixel
 * displays. It has to be somewhat refactored and finished. It should receive events from
 * the event manager and display UI screens according to them. It can receive button or
 * other events too and make an interactive UI that way.
 * It is important to keep in mind that all user interface modules should be programmed
 * in a way that would allow using many of them simultaneously (also with the CLI interface).
 * The only two dependencies should be a display (mandatory) and an event manager (optional).
 * This feature is planned to be finished in 1.1.0.
 */

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "port.h"
#include "task.h"
#include "u_assert.h"
#include "u_log.h"

#include "ui_320240.h"

#include "qdl_primitives.h"
#include "images/hourglass.c"

char ubload_version_banner[50];
char ubload_version_version[50];
qdlWidget ubload_version = {
	.shape = QDL_GROUP,
	.position = {0, 0},
	.size = {320, 240},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 0},
				.size = {320, 12},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &font_small,
					.text = ubload_version_banner,
				},
			},
			&(qdlWidget) {
				.shape = QDL_TEXT,
				.position = {0, 12},
				.size = {320, 16},
				.properties.text = {
					.text_color = COLOR_BLACK,
					.text_font = &font_small,
					.text = ubload_version_version,
				},
			},
			NULL
		},
	}
};

qdlWidget progress_indicator1 = {
	.shape = QDL_ELLIPSE,
	.position = {11, 0},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};

qdlWidget progress_indicator3 = {
	.shape = QDL_ELLIPSE,
	.position = {22, 11},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator5 = {
	.shape = QDL_ELLIPSE,
	.position = {11, 22},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator7 = {
	.shape = QDL_ELLIPSE,
	.position = {0, 11},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator2 = {
	.shape = QDL_ELLIPSE,
	.position = {19, 3},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator4 = {
	.shape = QDL_ELLIPSE,
	.position = {19, 19},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator6 = {
	.shape = QDL_ELLIPSE,
	.position = {3, 19},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};
qdlWidget progress_indicator8 = {
	.shape = QDL_ELLIPSE,
	.position = {3, 3},
	.size = {8, 8},
	.properties.ellipse = {
		.fill_color = COLOR_BLACK,
		.border_color = COLOR_WHITE,
		.border_width = 0,
	},
};

qdlWidget progress_indicator = {
	.shape = QDL_GROUP,
	.position = {144, 104},
	.size = {32, 32},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&progress_indicator1,
			&progress_indicator2,
			&progress_indicator3,
			&progress_indicator4,
			&progress_indicator5,
			&progress_indicator6,
			&progress_indicator7,
			&progress_indicator8,
			NULL
		},
	}
};

qdlWidget progressbar_hourglass = {
	.shape = QDL_IMAGE,
	.position = {0, 0},
	.size = {16, 28},
	.properties.image = {
		.color = COLOR_BLACK,
		.data = images_hourglass_small_data
	},
};

qdlWidget progressbar_log_text = {
	.shape = QDL_TEXT,
	.position = {24, 0},
	.size = {256, 16},
	.properties.text = {
		.text_color = COLOR_BLACK,
		.text_font = &dejavu_sans_bold_10,
		.text = "Authenticating firmware...",
	},
};

qdlWidget progressbar_progress = {
	.shape = QDL_RECT,
	.position = {24, 14},
	.size = {176, 12},
	.properties.rect = {
		.border_color = COLOR_BLACK,
		.border_width = 1,
		.fill_color = COLOR_WHITE,
	},
};

qdlWidget progressbar_progress_inner = {
	.shape = QDL_RECT,
	.position = {26, 16},
	.size = {72, 8},
	.properties.rect = {
		.border_color = COLOR_BLACK,
		.border_width = 0,
		.fill_color = COLOR_BLACK,
	},
};


qdlWidget progressbar = {
	.shape = QDL_GROUP,
	.size = {320, 28},
	.position = {48, 80},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&progressbar_hourglass,
			&progressbar_log_text,
			&progressbar_progress,
			&progressbar_progress_inner,
			NULL
		},
	},
};



qdlWidget boot_wait_screen = {
	.shape = QDL_GROUP,
	.size = {320, 240},
	.properties.group = {
		.children = &(qdlWidget*[]) {
			&ubload_version,
			//~ &progress_indicator,
			&progressbar,
			NULL
		},
	},
};

struct qdl_ellipse_properties progress_indicator_segment_properties[8] = {
	{ COLOR_WHITE, COLOR_WHITE, 0},
	{ COLOR_WHITE, COLOR_BLACK, 3},
	{ COLOR_WHITE, COLOR_BLACK, 2},
	{ COLOR_WHITE, COLOR_BLACK, 1},
	{ COLOR_WHITE, COLOR_BLACK, 0},
	{ COLOR_WHITE, COLOR_BLACK, 0},
	{ COLOR_WHITE, COLOR_BLACK, 0},
	{ COLOR_WHITE, COLOR_BLACK, 0},
};


static void progress_indicator_update(uint8_t progress) {
	progress_indicator1.properties.ellipse = progress_indicator_segment_properties[(progress) % 8];
	progress_indicator2.properties.ellipse = progress_indicator_segment_properties[(progress + 1) % 8];
	progress_indicator3.properties.ellipse = progress_indicator_segment_properties[(progress + 2) % 8];
	progress_indicator4.properties.ellipse = progress_indicator_segment_properties[(progress + 3) % 8];
	progress_indicator5.properties.ellipse = progress_indicator_segment_properties[(progress + 4) % 8];
	progress_indicator6.properties.ellipse = progress_indicator_segment_properties[(progress + 5) % 8];
	progress_indicator7.properties.ellipse = progress_indicator_segment_properties[(progress + 6) % 8];
	progress_indicator8.properties.ellipse = progress_indicator_segment_properties[(progress + 7) % 8];
}


static void version_update(void) {
	snprintf(ubload_version_banner, sizeof(ubload_version_banner), "%s", PORT_BANNER);
	snprintf(ubload_version_version, sizeof(ubload_version_version), "%s, version %s", PORT_NAME, UBLOAD_VERSION);
}

static void ui_320240_task(void *p) {
	struct ui_320240 *self = (struct ui_320240 *)p;

	int8_t progress = 0;

	while (1) {
		progress--;
		if (progress < 0) {
			progress = 7;
		}
		progress_indicator_update(progress);
		vTaskDelay(200);
	}
}


int32_t ui_320240_init(struct ui_320240 *self, struct interface_display *display) {
	if (u_assert(self != NULL) ||
	    u_assert(display != NULL)) {
		return UI_320240_INIT_FAILED;
	}

	self->display = display;
	interface_display_set_screen(display, &boot_wait_screen);
	xTaskCreate(ui_320240_task, "ui320240", configMINIMAL_STACK_SIZE + 256, (void *)self, 1, NULL);
	version_update();

	return UI_320240_INIT_OK;
}

