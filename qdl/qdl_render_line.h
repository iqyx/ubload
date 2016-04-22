#pragma once

#include "qdl_primitives.h"

qdlResult qdl_render_line(qdlColor c[], const qdlPos posx1, const qdlPos posx2, const qdlPos posy, const qdlWidget *w);
#define QDL_RENDER_LINE_NOOP 1
#define QDL_RENDER_LINE_OK 0
#define QDL_RENDER_LINE_FAILED -1

