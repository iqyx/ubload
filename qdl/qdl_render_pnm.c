#include <stdio.h>

#include "qdl_primitives.h"
#include "qdl_render_line.h"
#include "qdl_render_pnm.h"


int qdl_render_pnm(qdlWidget *w, qdlSize width, qdlSize height, int zoom) {

	qdlColor c[width];

	printf("P6\n%d %d\n255\n", width * zoom, height * zoom);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			c[x] = (qdlColor)COLOR_WHITE;
		}

		qdl_render_line(c, 0, width - 1, y, w);

		for (int i = 0; i < zoom; i++) {
			for (int x = 0; x < width; x++) {
				for (int i = 0; i < zoom; i++) {
					printf("%c%c%c", (c[x].r / 4) * 4, (c[x].g / 4) * 4, (c[x].b / 4) * 4);
				}
			}
		}
	}

	return 0;
}

