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

#ifndef _GDYNTEXTCOMPAT_H_
#define _GDYNTEXTCOMPAT_H_

#include "gdyntext.h"

/* parasites for compatibility with GDynText 1.4.4 and later versions */
#define GDYNTEXT_PARASITE_144					"plug_in_gdyntext/data"

/* parasites for compatibility with GDynText 1.3.1 - 1.4.3 */
#define GDYNTEXT_PARASITE_131					"gdyntext-parasite"

/* parasites for compatibility with GDynText 1.3.0 */
#define GDYNTEXT_PARASITE_130_PFX					"gdyntext_parasite"
#define GDYNTEXT_PARASITE_130_MAGIC				GDYNTEXT_PARASITE_130_PFX"_magic"
#define GDYNTEXT_PARASITE_130_TEXT				GDYNTEXT_PARASITE_130_PFX"_text"
#define GDYNTEXT_PARASITE_130_FONT_FAMILY	GDYNTEXT_PARASITE_130_PFX"_font_family"
#define GDYNTEXT_PARASITE_130_FONT_STYLE	GDYNTEXT_PARASITE_130_PFX"_font_style"
#define GDYNTEXT_PARASITE_130_FONT_SIZE		GDYNTEXT_PARASITE_130_PFX"_font_size"
#define GDYNTEXT_PARASITE_130_FONT_METRIC	GDYNTEXT_PARASITE_130_PFX"_font_metric"
#define GDYNTEXT_PARASITE_130_FONT_COLOR	GDYNTEXT_PARASITE_130_PFX"_font_color"
#define GDYNTEXT_PARASITE_130_ANTIALIAS		GDYNTEXT_PARASITE_130_PFX"_antialias"
#define GDYNTEXT_PARASITE_130_ALIGNMENT		GDYNTEXT_PARASITE_130_PFX"_alignment"
#define GDYNTEXT_PARASITE_130_ROTATION		GDYNTEXT_PARASITE_130_PFX"_rotation"
#define GDYNTEXT_PARASITE_130_PREVIEW			GDYNTEXT_PARASITE_130_PFX"_preview"


/* parasite blocks */
typedef enum
{
	C_TEXT				= 0,
	C_FONT_FAMILY	= 1,
	C_FONT_SIZE		= 2,
	C_FONT_SIZE_T	= 3,
	C_FONT_COLOR	= 4,
	C_ANTIALIAS		= 5,
	C_ALIGNMENT		= 6,
	C_ROTATION		= 7,
	C_FONT_STYLE	= 8,
	C_SPACING			= 9
} GDTCompatBlock;


/* gimp-1.1.14+ function names reorganization forgotten something */
#if defined(GIMP_HAVE_PARASITES) && GIMP_MICRO_VERSION < 14
#	warning Replacing missing function 'gimp_drawable_parasite_find'
#	define gimp_drawable_parasite_find gimp_drawable_find_parasite
#endif


gboolean gdt_compat_load(GdtVals *data);

#endif /* _GDYNTEXTCOMPAT_H_ */

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
