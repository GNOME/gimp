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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DRAWABLE_PVT_H__
#define __DRAWABLE_PVT_H__

#include <gtk/gtkdata.h>
#include "tile_manager.h"
#include "temp_buf.h"
#include "canvas.h"

struct _GimpDrawable
{
  GtkData data;

  char *name;				/* name of drawable */
  TileManager *tiles;			/* tiles for drawable data */
  Canvas * canvas;
  int visible;				/* controls visibility */
  int width, height;			/* size of drawable */
  int offset_x, offset_y;		/* offset of layer in image */

  Tag tag;
  
  int bytes;				/* bytes per pixel */
  int dirty;				/* dirty bit */
  int ID;				/* provides a unique ID */
  int gimage_ID;			/* ID of gimage owner */
  int type;				/* type of drawable */
  int has_alpha;			/* drawable has alpha */

  /*  Preview variables  */
  TempBuf *preview;			/* preview of the channel */
  int preview_valid;			/* is the preview valid? */
};

struct _GimpDrawableClass
{
  GtkDataClass parent_class;

  void (*invalidate_preview) (GtkObject *);
};

#endif /* __DRAWABLE_PVT_H__ */
