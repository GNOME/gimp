/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999,2000 Marco Lamberto <lm@geocities.com>
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

#define GDYNTEXT_VERSION				"1.5.0"
#define GDYNTEXT_MAGIC					"GDT10"
#define GDYNTEXT_WEB_PAGE				"http://www.geocities.com/Tokyo/1474/gimp/"
#define MAX_TEXT_SIZE						(1024 << 6)

#define GDYNTEXT_PARASITE							"plug_in_gdyntext/data"


/* version detection macros */
#define GDT_MAGIC_REV(lname)	(atoi(lname + 3))
#define GDT_REV()							((int)(atof(GDYNTEXT_VERSION) * 10))


typedef enum
{
	LEFT		= 0,
	CENTER	= 1,
	RIGHT		= 2
} GdtAlign;

typedef enum
{
	TEXT						= 0,
	ANTIALIAS				= 1,
	ALIGNMENT				= 2,
	ROTATION				= 3,
	LINE_SPACING		= 4,
	COLOR						= 5,
	LAYER_ALIGNMENT	= 6,
	XLFD						= 7,
} GDTBlock;

typedef enum
{
	XLFD_NONE						= 0,
	XLFD_FOUNDRY				= 1,
	XLFD_FAMILY					= 2,
	XLFD_WEIGHT					= 3,
	XLFD_SLANT					= 4,
	XLFD_SET_WIDTH			= 5,
	XLFD_ADD_STYLE			= 6,
	XLFD_PIXEL_SIZE			= 7,
	XLFD_POINT_SIZE			= 8,
	XLFD_RES_X					= 9,
	XLFD_RES_Y					= 10,
	XLFD_SPACING				= 11,
	XLFD_AVERAGE_WIDTH	= 12,
	XLFD_REGISTRY				= 13,
	XLFD_ENCODING				= 14
} XLFDBlock;

typedef enum
{
	LA_NONE						= 0,
	LA_BOTTOM_LEFT		= 1,
	LA_BOTTOM_CENTER	= 2,
	LA_BOTTOM_RIGHT		= 3,
	LA_MIDDLE_LEFT		= 4,
	LA_CENTER					= 5,
	LA_MIDDLE_RIGHT		= 6,
	LA_TOP_LEFT				= 7,
	LA_TOP_CENTER			= 8,
	LA_TOP_RIGHT			= 9,
} LayerAlignments;


typedef struct {
/* TRUE if the structure is correctly loaded throught gimp_get_data */
	gboolean	valid;
/* Plug-in values */
	gboolean	new_layer;
	gint32		image_id;
	gint32		layer_id;
	gint32		drawable_id;
	gchar			text[MAX_TEXT_SIZE];
	gchar			xlfd[1024];
	gint32		color;
	gboolean	antialias;
	GdtAlign	alignment;
	gint			rotation;
	gint			line_spacing;
	gint			layer_alignment;
	gboolean	change_layer_name;	/* if TRUE force parasites use + label change */
/* GUI stuff */
	GList			*messages;			/* FIXME: replace this stuff through a status bar */
	gboolean	preview;
} GdtVals;


#include "gdyntext_ui.h"
#include "gdyntextcompat.h"


void gdt_load(GdtVals *data);
void gdt_save(GdtVals *data);
void gdt_render_text(GdtVals *data);
void gdt_render_text_p(GdtVals *data, gboolean show_progress);

#endif /* _GDYNTEXT_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
