/* GIMP - The GNU Image Manipulation Program
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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"
#include "base/tile-manager-preview.h"

#include "gimpimage.h"
#include "gimpimage-preview.h"
#include "gimpprojection.h"


void
gimp_image_get_preview_size (GimpViewable *viewable,
                             gint          size,
                             gboolean      is_popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  gimp_viewable_calc_preview_size (image->width,
                                   image->height,
                                   size,
                                   size,
                                   dot_for_dot,
                                   image->xresolution,
                                   image->yresolution,
                                   width,
                                   height,
                                   NULL);
}

gboolean
gimp_image_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (image->width > width || image->height > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (image->width,
                                       image->height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = image->width;
          *popup_height = image->height;
        }

      return TRUE;
    }

  return FALSE;
}

TempBuf *
gimp_image_get_preview (GimpViewable *viewable,
                        GimpContext  *context,
                        gint          width,
                        gint          height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (image->preview                  &&
      image->preview->width  == width &&
      image->preview->height == height)
    {
      /*  The easy way  */
      return image->preview;
    }
  else
    {
      /*  The hard way  */
      if (image->preview)
        temp_buf_free (image->preview);

      image->preview = gimp_image_get_new_preview (viewable, context,
                                                        width, height);

      return image->preview;
    }
}

TempBuf *
gimp_image_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpImage   *image = GIMP_IMAGE (viewable);
  TileManager *tiles;
  gdouble      scale_x;
  gdouble      scale_y;
  gint         level;

  scale_x = (gdouble) width  / (gdouble) image->width;
  scale_y = (gdouble) height / (gdouble) image->height;

  level = gimp_projection_get_level (image->projection, scale_x, scale_y);
  tiles = gimp_projection_get_tiles_at_level (image->projection, level);

  return tile_manager_get_preview (tiles, width, height);
}
