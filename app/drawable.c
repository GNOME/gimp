
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
#include "palette.h"
#include "gimpimageF.h"
#include "gdisplay.h"
#include "undo.h"

int
drawable_ID (GimpDrawable *drawable)
{
  return drawable->ID;
}

void
drawable_fill (GimpDrawable *drawable, int fill_type){
	guchar r,g,b,a;
	a=255;
	switch(fill_type){
	case BACKGROUND_FILL:
		palette_get_background(&r, &g, &b);
		break;
	case WHITE_FILL:
		r=g=b=255;
		break;
	case TRANSPARENT_FILL:
		a=r=g=b=0;
		break;
	case NO_FILL:
		return;
	}
	gimp_drawable_fill(drawable,r,g,b,a);
	drawable_update (drawable, 0, 0,
			 gimp_drawable_width (drawable),
			 gimp_drawable_height (drawable));
}

void
drawable_update (GimpDrawable *drawable, int x, int y, int w, int h)
{
  GimpImage *gimage;
  int offset_x, offset_y;

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
		      int x1, int y1, int x2, int y2, 
		      TileManager *tiles, int sparse)
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

