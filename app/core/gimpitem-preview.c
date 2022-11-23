/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligmaimage.h"
#include "ligmaitem.h"
#include "ligmaitem-preview.h"


/*  public functions  */

void
ligma_item_get_preview_size (LigmaViewable *viewable,
                            gint          size,
                            gboolean      is_popup,
                            gboolean      dot_for_dot,
                            gint         *width,
                            gint         *height)
{
  LigmaItem  *item  = LIGMA_ITEM (viewable);
  LigmaImage *image = ligma_item_get_image (item);

  if (image && ! image->ligma->config->layer_previews && ! is_popup)
    {
      *width  = size;
      *height = size;
      return;
    }

  if (image && ! is_popup)
    {
      gdouble xres;
      gdouble yres;

      ligma_image_get_resolution (image, &xres, &yres);

      ligma_viewable_calc_preview_size (ligma_image_get_width  (image),
                                       ligma_image_get_height (image),
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
      ligma_viewable_calc_preview_size (ligma_item_get_width  (item),
                                       ligma_item_get_height (item),
                                       size,
                                       size,
                                       dot_for_dot, 1.0, 1.0,
                                       width,
                                       height,
                                       NULL);
    }
}

gboolean
ligma_item_get_popup_size (LigmaViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  LigmaItem  *item  = LIGMA_ITEM (viewable);
  LigmaImage *image = ligma_item_get_image (item);

  if (image && ! image->ligma->config->layer_previews)
    return FALSE;

  if (ligma_item_get_width  (item) > width ||
      ligma_item_get_height (item) > height)
    {
      gboolean scaling_up;
      gdouble  xres = 1.0;
      gdouble  yres = 1.0;

      if (image)
        ligma_image_get_resolution (image, &xres, &yres);

      ligma_viewable_calc_preview_size (ligma_item_get_width  (item),
                                       ligma_item_get_height (item),
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
          *popup_width  = ligma_item_get_width  (item);
          *popup_height = ligma_item_get_height (item);
        }

      return TRUE;
    }

  return FALSE;
}
