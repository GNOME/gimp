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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "apptypes.h"

#include "drawable.h"
#include "gimpcontext.h"
#include "gdisplay.h"
#include "undo.h"


void
drawable_fill (GimpDrawable *drawable,
	       GimpFillType  fill_type)
{
  GimpRGB color;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  color.a = 1.0;

  switch (fill_type)
    {
    case FOREGROUND_FILL:
      gimp_context_get_foreground (NULL, &color);
      break;

    case BACKGROUND_FILL:
      gimp_context_get_background (NULL, &color);
      break;

    case WHITE_FILL:
      gimp_rgb_set (&color, 1.0, 1.0, 1.0);
      break;

    case TRANSPARENT_FILL:
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);
      break;

    case NO_FILL:
      return;

    default:
      g_warning ("drawable_fill(): unknown fill type");
      return;
    }

  gimp_drawable_fill (drawable, &color);

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

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_drawable_gimage (drawable);
  g_return_if_fail (gimage != NULL);

  gimp_drawable_offsets (drawable, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;
  gdisplays_update_area (gimage, x, y, w, h);

  /*  invalidate the preview  */
  gimp_drawable_invalidate_preview (drawable, FALSE);
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
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (! tiles)
    undo_push_image (drawable->gimage, drawable, 
		     x1, y1, x2, y2);
  else
    undo_push_image_mod (drawable->gimage, drawable, 
			 x1, y1, x2, y2, tiles, sparse);
}
