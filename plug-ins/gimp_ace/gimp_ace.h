/* $Id$ */

#include "glace.h"

typedef struct {
	gdouble strength;
	gdouble bradj;
	gdouble coefftol;
	gdouble smoothing;
	guint iterations;
	guint wsize;
	Glace_ColorMethods color_method;
	gboolean link;
} AceValues;
