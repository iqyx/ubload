#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


/** qdl user-defined font is just array of bytes */
typedef const uint8_t qdlFont[];

typedef int32_t qdlResult;

typedef int16_t qdlSize;
typedef int16_t qdlPos;
#define false 0

typedef struct qdl_point_t {
	qdlPos x, y;
} qdlPoint;

typedef struct qdl_color_t {
	uint8_t r, g, b, a;
} qdlColor;

struct qdl_widget_t;
typedef struct qdl_widget_t qdlWidget;

enum qdl_widget_shape {
	/**
	 * Basic widget shape which draws nothing. This is the default value.
	 */
	QDL_WIDGET = 0,

	/**
	 * To optimize drawing and avoid traversing all primitives in the list,
	 * groups are used to set drawing boundaries of their children.
	 * Widgets outside the boundaries will not be drawn.
	 */
	QDL_GROUP,

	/**
	 * Filled rectangle. Fill and/or border can be hidden using 0 opacity.
	 */
	QDL_RECT,

	/**
	* Filled ellipse. Fill and/or border can be hidden using 0 opacity.
	*/
	QDL_ELLIPSE,
	QDL_HISTOGRAM,
	QDL_TEXT,
	QDL_IMAGE,


};

struct qdl_widget_properties {
	/* Empty widget has no additional properties. */
};

struct qdl_group_properties {
	qdlWidget *(*children)[];
};

struct qdl_rect_properties {
	qdlColor border_color;
	qdlColor fill_color;
	qdlSize border_width;
};

struct qdl_ellipse_properties {
	qdlColor border_color;
	qdlColor fill_color;
	qdlSize border_width;
};

struct qdl_histogram_properties {
	qdlColor bar_color;
	qdlSize bar_width;
	qdlSize data_size;
	qdlPos *data;
};

struct qdl_text_properties {
	qdlColor text_color;
	const qdlFont *text_font;
	char *text;
};

struct qdl_image_properties {
	const uint8_t *data;
	qdlColor color;
};

union qdl_properties {
	struct qdl_widget_properties widget;
	struct qdl_group_properties group;
	struct qdl_rect_properties rect;
	struct qdl_ellipse_properties ellipse;
	struct qdl_histogram_properties histogram;
	struct qdl_text_properties text;
	struct qdl_image_properties image;
};

struct qdl_widget_t {
	enum qdl_widget_shape shape;

	/**
	 * Position determines where the widget will be drawn in the parent's
	 * coordinates.
	 */
	qdlPoint position;

	/**
	 * Size of the widget. Must be non-negative.
	 */
	qdlPoint size;

	/**
	 * If the widget is set as hidden, it is not drawn. This saves some
	 * computational resources.
	 */
	bool hidden;

	/**
	 * Each widget shape can have some additional properties, such as colors,
	 * line widths, etc. They are specified using @p qdl_properties union.
	 */
	union qdl_properties properties;
};



/**
 * Basic colors
 */
#define COLOR_WHITE {255, 255, 255, 255}
#define COLOR_SILVER {160, 160, 160, 255}
#define COLOR_GRAY {96, 96, 96, 255}
#define COLOR_BLACK {0, 0, 0, 255}


/**
 * Fonts
 */
extern qdlFont dejavu_sans_book_10;
extern qdlFont dejavu_sans_bold_10;
extern qdlFont dejavu_sans_book_12;
extern qdlFont dejavu_sans_bold_12;
extern qdlFont dejavu_sans_bold_24;
extern qdlFont minecraftia;
extern qdlFont font_small;
extern qdlFont pixelmix;
extern qdlFont pixelmix_bold;


/**
 * Text with slightly more advanced user defined fonts.
 */
//~ typedef struct qdl_text_t {
	//~ qdlWidget widget;

	//~ qdlColor text_color;
	//~ qdlFont *text_font;
	//~ char *text;
//~ } qdlText;


//~ int qdl_text_get_width(char *ch, qdlFont *font);

