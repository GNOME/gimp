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
#ifndef __GIMPDRAWABLEP_H__
#define __GIMPDRAWABLEP_H__

#include "gimpobjectP.h"
#include "gimpdrawable.h"

struct _GimpDrawable
{
  GimpObject data;

  char *name;				/* name of drawable */
  TileManager *tiles;			/* tiles for drawable data */
  int visible;				/* controls visibility */
  int width, height;			/* size of drawable */
  int offset_x, offset_y;		/* offset of layer in image */

  int bytes;				/* bytes per pixel */
  int dirty;				/* dirty bit */
  int ID;				/* provides a unique ID */
  GimpImage* gimage;			/* gimage owner */
  int type;				/* type of drawable */
  int has_alpha;			/* drawable has alpha */

  GSList *parasites;                    /* Plug-in parasite data   */

  /*  Preview variables  */
  TempBuf *preview;			/* preview of the channel */
  int preview_valid;			/* is the preview valid? */
};

struct _GimpDrawableClass
{
  GimpObjectClass parent_class;

  void (*invalidate_preview) (GtkObject *);
};

typedef struct _GimpDrawableClass GimpDrawableClass;

#define GIMP_DRAWABLE_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE, GimpDrawableClass))

#define GIMP_IS_DRAWABLE_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE))

void gimp_drawable_configure (GimpDrawable *, GimpImage*,
			      gint, gint, gint, gchar*);

#endif /* __GIMPDRAWABLEP_H__ */
