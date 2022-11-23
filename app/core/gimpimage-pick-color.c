/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmacontainer.h"
#include "ligmadrawable.h"
#include "ligmaimage.h"
#include "ligmaimage-new.h"
#include "ligmaimage-pick-color.h"
#include "ligmalayer.h"
#include "ligmapickable.h"


gboolean
ligma_image_pick_color (LigmaImage   *image,
                       GList       *drawables,
                       gint         x,
                       gint         y,
                       gboolean     show_all,
                       gboolean     sample_merged,
                       gboolean     sample_average,
                       gdouble      average_radius,
                       const Babl **sample_format,
                       gpointer     pixel,
                       LigmaRGB     *color)
{
  LigmaImage    *pick_image = NULL;
  LigmaPickable *pickable;
  GList        *iter;
  gboolean      result;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (ligma_item_get_image (iter->data) == image,
                            FALSE);
    }

  if (sample_merged && g_list_length (drawables) == 1)
    {
      if ((LIGMA_IS_LAYER (drawables->data) &&
           ligma_image_get_n_layers (image) == 1) ||
          (LIGMA_IS_CHANNEL (drawables->data) &&
           ligma_image_get_n_channels (image) == 1))
        {
          /* Let's add a special exception when an image has only one
           * layer. This was useful in particular for indexed image as
           * it allows to pick the right index value even when "Sample
           * merged" is checked. There are more possible exceptions, but
           * we can't just take them all in considerations unless we
           * want to make code extra-complicated).
           * See #3041.
           */
          sample_merged = FALSE;
        }
    }

  if (sample_merged)
    {
      if (! show_all)
        pickable = LIGMA_PICKABLE (image);
      else
        pickable = LIGMA_PICKABLE (ligma_image_get_projection (image));
    }
  else
    {
      gboolean free_drawables = FALSE;

      if (! drawables)
        {
          drawables = ligma_image_get_selected_drawables (image);
          free_drawables = TRUE;
        }

      if (! drawables)
        return FALSE;

      if (g_list_length (drawables) == 1)
        {
          gint off_x, off_y;

          ligma_item_get_offset (LIGMA_ITEM (drawables->data), &off_x, &off_y);
          x -= off_x;
          y -= off_y;

          pickable = LIGMA_PICKABLE (drawables->data);
        }
      else /* length > 1 */
        {
          pick_image = ligma_image_new_from_drawables (image->ligma, drawables, FALSE, FALSE);
          ligma_container_remove (image->ligma->images, LIGMA_OBJECT (pick_image));

          if (! show_all)
            pickable = LIGMA_PICKABLE (pick_image);
          else
            pickable = LIGMA_PICKABLE (ligma_image_get_projection (pick_image));
          ligma_pickable_flush (pickable);
        }

      if (free_drawables)
        g_list_free (drawables);
    }

  /* Do *not* call ligma_pickable_flush() here because it's too expensive
   * to call it unconditionally each time e.g. the cursor view is updated.
   * Instead, call ligma_pickable_flush() in the callers if needed.
   */

  if (sample_format)
    *sample_format = ligma_pickable_get_format (pickable);

  result = ligma_pickable_pick_color (pickable, x, y,
                                     sample_average &&
                                     ! (show_all && sample_merged),
                                     average_radius,
                                     pixel, color);

  if (show_all && sample_merged)
    {
      const Babl *format    = babl_format ("RaGaBaA double");
      gdouble     sample[4] = {};

      if (! result)
        memset (pixel, 0, babl_format_get_bytes_per_pixel (*sample_format));

      if (sample_average)
        {
          GeglBuffer *buffer = ligma_pickable_get_buffer (pickable);
          gint        radius = floor (average_radius);

          ligma_gegl_average_color (buffer,
                                   GEGL_RECTANGLE (x - radius,
                                                   y - radius,
                                                   2 * radius + 1,
                                                   2 * radius + 1),
                                   FALSE, GEGL_ABYSS_NONE, format, sample);
        }

      if (! result || sample_average)
        ligma_pickable_pixel_to_srgb (pickable, format, sample, color);

      result = TRUE;
    }

  if (pick_image)
    g_object_unref (pick_image);

  return result;
}
