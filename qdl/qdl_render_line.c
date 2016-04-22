#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "qdl_render_line.h"

/**
 * @todo Move to the qdl line renderer (the other one is the frame renderer).
 *       This file should only containt functions for manipulating primitives.
 */
/**
 * @brief Render the whole line at once
 *
 * This function traverses given primitive widgets and tries to render them.
 * Widgets placed outside the rendering window or hidden widgets are ignored.
 * Groups are rendered recursively.
 *
 * @param c       Array of qdlColor (a single line) where the rendered objects
 *                will be saved.
 * @param posx1   Rendering starting position within the @p c array
 * @param posx2   Rendering stop position within the @p array
 * @param w       The qdlWidget to render
 *
 * @return QDL_RENDER_LINE_OK on success or
 *         QDL_RENDER_LINE_NOOP if no widgets can be rendered in the selected window or
 *         QDL_RENDER_FAILED if rendering failed.
 */
qdlResult qdl_render_line(qdlColor c[], const qdlPos posx1, const qdlPos posx2, const qdlPos posy, const qdlWidget *w) {
	assert(c != NULL);
	assert(w != NULL);

	/* The widget is not visible. */
	if (w->hidden) {
		return QDL_RENDER_LINE_NOOP;
	}

	/* Compute start and end of the rendering line for this widget. Widget
	 * size must be non-negative. */
	assert(w->size.x >= 0);
	assert(w->size.y >= 0);
	qdlPos ws_x1 = posx1 + w->position.x;
	qdlPos ws_x2 = posx1 + w->position.x + w->size.x;

	/* If the widget starts beyond the right edge of rendering window, return. */
	if ((ws_x1 > posx2) ||
	    /* or if the vertical position is outside rendering window, return */
	    (posy < w->position.y || posy > (w->position.y + w->size.y))) {
		return QDL_RENDER_LINE_NOOP;
	}

	/* If the widget spans beyond the rendering line, crop the rendering line. */
	if (ws_x1 < 0) {
		ws_x1 = 0;
	}
	if (ws_x2 > posx2) {
		ws_x2 = posx2;
	}

	switch (w->shape) {
		case QDL_WIDGET:
			break;

		case QDL_GROUP: {
			for (int i = 0; (*(w->properties.group.children))[i] != NULL; i++) {
				qdlResult res = qdl_render_line(c, ws_x1, ws_x2, posy - w->position.y, (*(w->properties.group.children))[i]);
				if (res == QDL_RENDER_LINE_FAILED) {
					return res;
				}
			}
			break;
		}

		case QDL_RECT: {
			for (int x = w->position.x; x <= (w->position.x + w->size.x); x++) {
				int posx = x + posx1;
				if (posx > posx2 || posx < posx1) {
					continue;
				}

				if ((posx < (posx1 + w->position.x + w->properties.rect.border_width)) ||
				    (posx > (posx1 + (w->position.x + w->size.x) - w->properties.rect.border_width)) ||
				    (posy < (w->position.y + w->properties.rect.border_width)) ||
				    (posy > ((w->position.y + w->size.y) - w->properties.rect.border_width))) {
					c[posx] = w->properties.rect.border_color;
				} else {
					c[posx] = w->properties.rect.fill_color;
				}
			}
			break;
		}

		case QDL_ELLIPSE: {
			qdlPos r = w->size.x / 2;
			if ((w->size.y / 2) < r) {
				r = w->size.y / 2;
			}

			qdlPos centerx = w->position.x + w->size.x / 2;
			qdlPos centery = w->position.y + w->size.y / 2;

			for (int x = w->position.x; x <= (w->position.x + w->size.x); x++) {
				int posx = x + posx1;
				if (posx > posx2 || posx < posx1) {
					continue;
				}

				qdlPos len = r - hypot(x - centerx, posy - centery) + 0.5;

				if (len > w->properties.ellipse.border_width) {
					c[posx] = w->properties.ellipse.fill_color;
				} else if (len <= w->properties.ellipse.border_width && len > 0) {
					c[posx] = w->properties.ellipse.border_color;
				}
			}
			break;
		}

		case QDL_HISTOGRAM: {
			for (int x = w->position.x; x <= (w->position.x + w->size.x); x++) {
				int posx = x + posx1;
				if (posx > posx2 || posx < posx1) {
					continue;
				}

				int data_pos = (x - w->position.x) / w->properties.histogram.bar_width;

				if (data_pos < 0 || data_pos >= w->properties.histogram.data_size) {
					continue;
				}

				if (posy > ((w->position.y + w->size.y) - w->properties.histogram.data[data_pos])) {
					c[posx] = w->properties.histogram.bar_color;
				}
			}

			break;
		}

		case QDL_TEXT: {

			qdlFont *font = w->properties.text.text_font;
			int font_length = (*font)[0];
			int x_offset = 0;

			const char *ch = w->properties.text.text;
			while (*ch) {
				//~ printf("char = %c\n", *ch);
				if (x_offset > w->size.x) {
					break;
				}
				for (int i = 0; i < font_length; i++) {
					if ((*font)[1 + i * 5] == *ch) {
						int font_offset = 1 + font_length * 5 + (*font)[1 + i * 5 + 3] * 256 + (*font)[1 + i * 5 + 4];
						int glyph_width = (*font)[1 + i * 5 + 1];
						int glyph_height = (*font)[1 + i * 5 + 2];

						if ((posy - w->position.y) > glyph_height) {
							continue;
						}

						for (int x = 0; x < glyph_width; x++) {
							if (((*font)[font_offset + (posy - w->position.y) * ((glyph_width + 7) / 8) + x / 8]) & (1 << (7 - x % 8))) {
								if (x_offset + x + w->position.x + posx1 <= posx2) {
									c[posx1 + w->position.x + x_offset + x] = w->properties.text.text_color;
								}
							}
						}
						x_offset += glyph_width - 1;
					}
				}
				ch++;
			}
			break;
		}
	}
	return QDL_RENDER_LINE_OK;
}

