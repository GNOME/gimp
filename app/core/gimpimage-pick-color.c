/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpprojection.h"


gboolean
gimp_image_pick_color (GimpImage     *gimage,
                       GimpDrawable  *drawable,
                       gint           x,
                       gint           y,
                       gboolean       sample_merged,
                       gboolean       sample_average,
                       gdouble        average_radius,
                       GimpImageType *sample_type,
                       GimpRGB       *color,
                       gint          *color_index)
{
  GimpImageType           my_sample_type;
  gboolean                is_indexed;
  GimpImagePickColorFunc  color_func;
  GimpObject             *color_obj;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (sample_merged || GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (! drawable ||
                        gimp_item_get_image (GIMP_ITEM (drawable)) == gimage,
                        FALSE);

  if (sample_merged)
    {
      my_sample_type = gimp_projection_get_image_type (gimage->projection);
      is_indexed     = FALSE;

      color_func = (GimpImagePickColorFunc) gimp_projection_get_color_at;
      color_obj  = GIMP_OBJECT (gimage->projection);
    }
  else
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
      x -= off_x;
      y -= off_y;

      my_sample_type = gimp_drawable_type (drawable);
      is_indexed     = gimp_drawable_is_indexed (drawable);

      color_func = (GimpImagePickColorFunc) gimp_drawable_get_color_at;
      color_obj  = GIMP_OBJECT (drawable);
    }

  if (sample_type)
    *sample_type = my_sample_type;

  return gimp_image_pick_color_by_func (color_obj, x, y, color_func,
                                        sample_average, average_radius,
                                        color, color_index);
}

gboolean
gimp_image_pick_color_by_func (GimpObject             *object,
                               gint                    x,
                               gint                    y,
                               GimpImagePickColorFunc  pick_color_func,
                               gboolean                sample_average,
                               gdouble                 average_radius,
                               GimpRGB                *color,
                               gint                   *color_index)
{
  guchar *col;

  if (! (col = pick_color_func (object, x, y)))
    return FALSE;

  if (sample_average)
    {
      gint    i, j;
      gint    count = 0;
      gint    color_avg[4] = { 0, 0, 0, 0 };
      guchar *tmp_col;
      gint    radius = (gint) average_radius;

      for (i = x - radius; i <= x + radius; i++)
	for (j = y - radius; j <= y + radius; j++)
	  if ((tmp_col = pick_color_func (object, i, j)))
	    {
	      count++;
	
	      color_avg[RED_PIX]   += tmp_col[RED_PIX];
	      color_avg[GREEN_PIX] += tmp_col[GREEN_PIX];
	      color_avg[BLUE_PIX]  += tmp_col[BLUE_PIX];
              color_avg[ALPHA_PIX] += tmp_col[ALPHA_PIX];

	      g_free (tmp_col);
	    }

      col[RED_PIX]   = (guchar) (color_avg[RED_PIX]   / count);
      col[GREEN_PIX] = (guchar) (color_avg[GREEN_PIX] / count);
      col[BLUE_PIX]  = (guchar) (color_avg[BLUE_PIX]  / count);
      col[ALPHA_PIX] = (guchar) (color_avg[ALPHA_PIX] / count);
    }

  if (color)
    gimp_rgba_set_uchar (color,
                         col[RED_PIX],
                         col[GREEN_PIX],
                         col[BLUE_PIX],
                         col[ALPHA_PIX]);

  if (color_index)
    *color_index = sample_average ? -1 : col[4];

  g_free (col);

  return TRUE;
}
