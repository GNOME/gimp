/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifndef _GDYNTEXT_H_
#define _GDYNTEXT_H_

#include "libgimp/gimpintl.h"

#define GDYNTEXT_VERSION				"1.4.3"
#define GDYNTEXT_MAGIC					"GDT09"
#define GDYNTEXT_WEB_PAGE				"http://www.geocities.com/Tokyo/1474/gimp/"
#define MAX_TEXT_SIZE						(64 * 1024)

#define GDYNTEXT_PARASITE							"gdyntext-parasite"

/* parasites for compatibility with GDynText 1.3.0 */
#define GDYNTEXT_PARASITE_PFX					"gdyntext_parasite"
#define GDYNTEXT_PARASITE_MAGIC				GDYNTEXT_PARASITE_PFX"_magic"
#define GDYNTEXT_PARASITE_TEXT				GDYNTEXT_PARASITE_PFX"_text"
#define GDYNTEXT_PARASITE_FONT_FAMILY	GDYNTEXT_PARASITE_PFX"_font_family"
#define GDYNTEXT_PARASITE_FONT_STYLE	GDYNTEXT_PARASITE_PFX"_font_style"
#define GDYNTEXT_PARASITE_FONT_SIZE		GDYNTEXT_PARASITE_PFX"_font_size"
#define GDYNTEXT_PARASITE_FONT_METRIC	GDYNTEXT_PARASITE_PFX"_font_metric"
#define GDYNTEXT_PARASITE_FONT_COLOR	GDYNTEXT_PARASITE_PFX"_font_color"
#define GDYNTEXT_PARASITE_ANTIALIAS		GDYNTEXT_PARASITE_PFX"_antialias"
#define GDYNTEXT_PARASITE_ALIGNMENT		GDYNTEXT_PARASITE_PFX"_alignment"
#define GDYNTEXT_PARASITE_ROTATION		GDYNTEXT_PARASITE_PFX"_rotation"
#define GDYNTEXT_PARASITE_PREVIEW			GDYNTEXT_PARASITE_PFX"_preview"


typedef enum
{
	LEFT		= 0,
	CENTER	= 1,
	RIGHT		= 2
} GdtAlign;


typedef struct {
/* TRUE if the structure is correctly loaded throught gimp_get_data */
	gboolean	valid;
/* Plug-in values */
	gboolean	new_layer;
	gint32		image_id;
	gint32		layer_id;
	gint32		drawable_id;
	gchar			text[MAX_TEXT_SIZE];
	gchar			font_family[1024];
	gchar			font_style[1024];
	gint32		font_size;
	gint			font_metric;
	gint32		font_color;
	gboolean	antialias;
	GdtAlign	alignment;
	gint			rotation;
	gint			spacing;
#ifdef GIMP_HAVE_PARASITES
	gboolean	change_layer_name;	/* if TRUE force parasites use + label change */
#endif
/* GUI stuff */
	GList			*messages;
	gboolean	preview;
} GdtVals;


void gdt_get_values(GdtVals *data);
void gdt_set_values(GdtVals *data);
void gdt_render_text(GdtVals *data);
void gdt_render_text_p(GdtVals *data, gboolean show_progress);

#endif /* _GDYNTEXT_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
