/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */
#ifndef __TEXT_TOOL_H__
#define __TEXT_TOOL_H__

#include "tools.h"
#include "layerF.h"
#include "gimpimageF.h"
#include "gimpdrawableF.h"

typedef enum {
  PIXELS,
  POINTS
} SizeType;

#define SUPERSAMPLE  3

/*  text functions  */
Tool * tools_new_text  (void);
void   tools_free_text (Tool *);

int         text_get_extents (char *, char *, int *, int *, int *, int *);
GimpLayer * text_render      (GimpImage *, GimpDrawable *, int, int, char *,
			      char *, int, int);

#endif /* __TEXT_TOOL_H__ */
