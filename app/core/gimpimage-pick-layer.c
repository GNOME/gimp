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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-pick-layer.h"
#include "gimppickable.h"

#include "text/gimptextlayer.h"


GimpLayer *
gimp_image_pick_layer (GimpImage *image,
                       gint       x,
                       gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  all_layers = gimp_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer),
                                        x - off_x, y - off_y) > 0.25)
        {
          g_list_free (all_layers);

          return layer;
        }
    }

  g_list_free (all_layers);

  return NULL;
}

GimpLayer *
gimp_image_pick_layer_by_bounds (GimpImage *image,
                                 gint       x,
                                 gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  all_layers = gimp_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      if (gimp_item_is_visible (GIMP_ITEM (layer)))
        {
          gint off_x, off_y;
          gint width, height;

          gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);
          width  = gimp_item_get_width  (GIMP_ITEM (layer));
          height = gimp_item_get_height (GIMP_ITEM (layer));

          if (x >= off_x        &&
              y >= off_y        &&
              x < off_x + width &&
              y < off_y + height)
            {
              g_list_free (all_layers);

              return layer;
            }
        }
    }

  g_list_free (all_layers);

  return NULL;
}

GimpTextLayer *
gimp_image_pick_text_layer (GimpImage *image,
                            gint       x,
                            gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  all_layers = gimp_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       off_x, off_y;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      if (GIMP_IS_TEXT_LAYER (layer) &&
          x >= off_x &&
          y >= off_y &&
          x <  off_x + gimp_item_get_width  (GIMP_ITEM (layer)) &&
          y <  off_y + gimp_item_get_height (GIMP_ITEM (layer)) &&
          gimp_item_is_visible (GIMP_ITEM (layer)))
        {
          g_list_free (all_layers);

          return GIMP_TEXT_LAYER (layer);
        }
      else if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer),
                                             x - off_x, y - off_y) > 0.25)
        {
          /*  a normal layer covers any possible text layers below,
           *  bail out
           */

          break;
        }
    }

  g_list_free (all_layers);

  return NULL;
}
