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
#include "drawable.h"
#include "drawable_pvt.h"
#include "gimpcontext.h"
#include "gimpimageF.h"
#include "gdisplay.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

gint
drawable_ID (GimpDrawable *drawable)
{
  if (drawable)
    return drawable->ID;
  else
    g_warning ("drawable_ID called on a NULL pointer");

  return 0;
}

void
drawable_fill (GimpDrawable *drawable,
	       GimpFillType  fill_type)
{
  guchar r, g, b, a;

  a = 255;

  switch (fill_type)
    {
    case FOREGROUND_FILL:
      gimp_context_get_foreground (NULL, &r, &g, &b);
      break;

    case BACKGROUND_FILL:
      gimp_context_get_background (NULL, &r, &g, &b);
      break;

    case WHITE_FILL:
      r = g = b = 255;
      break;

    case TRANSPARENT_FILL:
      a = r = g = b = 0;
      break;

    case NO_FILL:
      return;

    default:
      g_warning (_("drawable_fill called with unknown fill type"));
      a = r = g = b = 0;
      break;
    }

  gimp_drawable_fill (drawable,r,g,b,a);
  
  drawable_update (drawable, 0, 0,
		   gimp_drawable_width  (drawable),
		   gimp_drawable_height (drawable));
}

void
drawable_update (GimpDrawable *drawable,
		 gint          x,
		 gint          y,
		 gint          w,
		 gint          h)
{
  GimpImage *gimage;
  gint offset_x, offset_y;

  if (! drawable)
    return;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  gimp_drawable_offsets (drawable, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;
  gdisplays_update_area (gimage, x, y, w, h);

  /*  invalidate the preview  */
  gimp_drawable_invalidate_preview (drawable);
}

void
drawable_apply_image (GimpDrawable *drawable, 
		      gint          x1,
		      gint          y1,
		      gint          x2,
		      gint          y2, 
		      TileManager  *tiles,
		      gint          sparse)
{
  if (drawable)
    {
      if (! tiles)
	undo_push_image (drawable->gimage, drawable, 
			 x1, y1, x2, y2);
      else
	undo_push_image_mod (drawable->gimage, drawable, 
			     x1, y1, x2, y2, tiles, sparse);
    }
}
