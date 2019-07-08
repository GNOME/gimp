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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitem-preview.h"


/*  public functions  */

void
gimp_item_get_preview_size (GimpViewable *viewable,
                            gint          size,
                            gboolean      is_popup,
                            gboolean      dot_for_dot,
                            gint         *width,
                            gint         *height)
{
  GimpItem  *item  = GIMP_ITEM (viewable);
  GimpImage *image = gimp_item_get_image (item);

  if (image && ! image->gimp->config->layer_previews && ! is_popup)
    {
      *width  = size;
      *height = size;
      return;
    }

  if (image && ! is_popup)
    {
      gdouble xres;
      gdouble yres;

      gimp_image_get_resolution (image, &xres, &yres);

      gimp_viewable_calc_preview_size (gimp_image_get_width  (image),
                                       gimp_image_get_height (image),
                                       size,
                                       size,
                                       dot_for_dot,
                                       xres,
                                       yres,
                                       width,
                                       height,
                                       NULL);
    }
  else
    {
      gimp_viewable_calc_preview_size (gimp_item_get_width  (item),
                                       gimp_item_get_height (item),
                                       size,
                                       size,
                                       dot_for_dot, 1.0, 1.0,
                                       width,
                                       height,
                                       NULL);
    }
}

gboolean
gimp_item_get_popup_size (GimpViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  GimpItem  *item  = GIMP_ITEM (viewable);
  GimpImage *image = gimp_item_get_image (item);

  if (image && ! image->gimp->config->layer_previews)
    return FALSE;

  if (gimp_item_get_width  (item) > width ||
      gimp_item_get_height (item) > height)
    {
      gboolean scaling_up;
      gdouble  xres = 1.0;
      gdouble  yres = 1.0;

      if (image)
        gimp_image_get_resolution (image, &xres, &yres);

      gimp_viewable_calc_preview_size (gimp_item_get_width  (item),
                                       gimp_item_get_height (item),
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot,
                                       xres,
                                       yres,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = gimp_item_get_width  (item);
          *popup_height = gimp_item_get_height (item);
        }

      return TRUE;
    }

  return FALSE;
}
