/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimplayer.h"
#include "gimptempbuf.h"


/*  public functions  */

GimpTempBuf *
gimp_drawable_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height)
{
  GimpItem  *item  = GIMP_ITEM (viewable);
  GimpImage *image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  return gimp_drawable_get_sub_preview (GIMP_DRAWABLE (viewable),
                                        0, 0,
                                        gimp_item_get_width  (item),
                                        gimp_item_get_height (item),
                                        width,
                                        height);
}

const Babl *
gimp_drawable_get_preview_format (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  switch (gimp_drawable_get_base_type (drawable))
    {
    case GIMP_GRAY:
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");

    case GIMP_RGB:
    case GIMP_INDEXED:
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }

  g_return_val_if_reached (NULL);
}

GimpTempBuf *
gimp_drawable_get_sub_preview (GimpDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  GimpItem    *item;
  GimpImage   *image;
  GeglBuffer  *buffer;
  GimpTempBuf *preview;
  gdouble      scale;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_get_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  buffer = gimp_drawable_get_buffer (drawable);

  preview = gimp_temp_buf_new (dest_width, dest_height,
                               gimp_drawable_get_preview_format (drawable));

  scale = MIN ((gdouble) dest_width  / (gdouble) gegl_buffer_get_width  (buffer),
               (gdouble) dest_height / (gdouble) gegl_buffer_get_height (buffer));

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (src_x, src_y, dest_width, dest_height),
                   scale,
                   gimp_temp_buf_get_format (preview),
                   gimp_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  return preview;
}
