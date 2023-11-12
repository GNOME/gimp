/* GIMP - The GNU Image Manipulation Program
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-new.h"
#include "gimpimage-pick-color.h"
#include "gimplayer.h"
#include "gimppickable.h"


gboolean
gimp_image_pick_color (GimpImage   *image,
                       GList       *drawables,
                       gint         x,
                       gint         y,
                       gboolean     show_all,
                       gboolean     sample_merged,
                       gboolean     sample_average,
                       gdouble      average_radius,
                       const Babl **sample_format,
                       gpointer     pixel,
                       GeglColor  **color)
{
  GimpImage    *pick_image = NULL;
  GimpPickable *pickable;
  GList        *iter;
  gboolean      result;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (color != NULL && GEGL_IS_COLOR (*color), FALSE);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (gimp_item_get_image (iter->data) == image,
                            FALSE);
    }

  if (sample_merged && g_list_length (drawables) == 1)
    {
      if ((GIMP_IS_LAYER (drawables->data) &&
           gimp_image_get_n_layers (image) == 1) ||
          (GIMP_IS_CHANNEL (drawables->data) &&
           gimp_image_get_n_channels (image) == 1))
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
        pickable = GIMP_PICKABLE (image);
      else
        pickable = GIMP_PICKABLE (gimp_image_get_projection (image));
    }
  else
    {
      gboolean free_drawables = FALSE;

      if (! drawables)
        {
          drawables = gimp_image_get_selected_drawables (image);
          free_drawables = TRUE;
        }

      if (! drawables)
        return FALSE;

      if (g_list_length (drawables) == 1)
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawables->data), &off_x, &off_y);
          x -= off_x;
          y -= off_y;

          pickable = GIMP_PICKABLE (drawables->data);
        }
      else /* length > 1 */
        {
          pick_image = gimp_image_new_from_drawables (image->gimp, drawables, FALSE, FALSE);
          gimp_container_remove (image->gimp->images, GIMP_OBJECT (pick_image));

          if (! show_all)
            pickable = GIMP_PICKABLE (pick_image);
          else
            pickable = GIMP_PICKABLE (gimp_image_get_projection (pick_image));
          gimp_pickable_flush (pickable);
        }

      if (free_drawables)
        g_list_free (drawables);
    }

  /* Do *not* call gimp_pickable_flush() here because it's too expensive
   * to call it unconditionally each time e.g. the cursor view is updated.
   * Instead, call gimp_pickable_flush() in the callers if needed.
   */

  if (sample_format)
    *sample_format = gimp_pickable_get_format (pickable);

  result = gimp_pickable_pick_color (pickable, x, y,
                                     sample_average &&
                                     ! (show_all && sample_merged),
                                     average_radius,
                                     pixel, color);

  if (show_all && sample_merged)
    {
      GimpColorProfile *profile = gimp_image_get_color_profile (image);
      const Babl *space         = NULL;
      gdouble     sample[4]     = {};
      const Babl *format;

      if (profile)
        space = gimp_color_profile_get_space (profile,
                                              GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                              NULL);
      format = babl_format_with_space ("RaGaBaA double", space);

      if (! result)
        memset (pixel, 0, babl_format_get_bytes_per_pixel (*sample_format));

      if (sample_average)
        {
          GeglBuffer *buffer = gimp_pickable_get_buffer (pickable);
          gint        radius = floor (average_radius);

          gimp_gegl_average_color (buffer,
                                   GEGL_RECTANGLE (x - radius,
                                                   y - radius,
                                                   2 * radius + 1,
                                                   2 * radius + 1),
                                   FALSE, GEGL_ABYSS_NONE, format, sample);
        }

      if (! result || sample_average)
        gegl_color_set_pixel (*color, format, sample);

      result = TRUE;
    }

  if (pick_image)
    g_object_unref (pick_image);

  return result;
}
