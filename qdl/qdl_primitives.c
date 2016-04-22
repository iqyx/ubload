#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "qdl_primitives.h"

#include "fonts/dejavu_sans_book_10.c"
#include "fonts/dejavu_sans_bold_10.c"
#include "fonts/dejavu_sans_book_12.c"
#include "fonts/dejavu_sans_bold_12.c"
#include "fonts/dejavu_sans_bold_24.c"
#include "fonts/minecraftia.c"
#include "fonts/font_small.c"
#include "fonts/pixelmix.c"
#include "fonts/pixelmix_bold.c"



/*
int qdl_text_get_width(char *ch, qdlFont *font) {
	int font_length = (*font)[0];
	qdlSize width = 0;

	while (*ch) {
		for (int i = 0; i < font_length; i++) {
			if ((*font)[1 + i * 5] == *ch) {
				qdlSize glyph_width = (*font)[1 + i * 5 + 1];
				width += glyph_width - 1;
			}
		}
		ch++;
	}

	return width;
}

*/


