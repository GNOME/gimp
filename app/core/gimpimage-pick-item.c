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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpgrouplayer.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-pick-item.h"
#include "gimpimage-private.h"
#include "gimppickable.h"
#include "gimpsamplepoint.h"

#include "path/gimppath.h"
#include "path/gimpstroke.h"

#include "text/gimptextlayer.h"


GimpLayer *
gimp_image_pick_layer (GimpImage *image,
                       gint       x,
                       gint       y,
                       GimpLayer *previously_picked)
{
  GList *all_layers;
  GList *list;
  gint   off_x, off_y;
  gint   tries = 1;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  all_layers = gimp_image_get_layer_list (image);

  if (previously_picked)
    {
      gimp_item_get_offset (GIMP_ITEM (previously_picked), &off_x, &off_y);
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (previously_picked),
                                        x - off_x, y - off_y) <= 0.25)
        previously_picked = NULL;
      else
        tries++;
    }

  while (tries)
    {
      for (list = all_layers; list; list = g_list_next (list))
        {
          GimpLayer *layer = list->data;

          if (previously_picked)
            {
              /* Take the first layer with a pixel at given coordinates
               * after the previously picked one.
               */
              if (layer == previously_picked)
                previously_picked = NULL;
              continue;
            }

          gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

          if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer),
                                            x - off_x, y - off_y) > 0.25)
            {
              g_list_free (all_layers);

              return layer;
            }
        }
      tries--;
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

GimpPath *
gimp_image_pick_path (GimpImage *image,
                      gdouble    x,
                      gdouble    y,
                      gdouble    epsilon_x,
                      gdouble    epsilon_y)
{
  GimpPath *ret = NULL;
  GList    *all_path;
  GList    *list;
  gdouble   mindist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  all_path = gimp_image_get_path_list (image);

  for (list = all_path; list; list = g_list_next (list))
    {
      GimpPath *path = list->data;

      if (gimp_item_is_visible (GIMP_ITEM (path)))
        {
          GimpStroke *stroke = NULL;
          GimpCoords  coords = GIMP_COORDS_DEFAULT_VALUES;

          while ((stroke = gimp_path_stroke_get_next (path, stroke)))
            {
              gdouble dist;

              coords.x = x;
              coords.y = y;

              dist = gimp_stroke_nearest_point_get (stroke, &coords, 1.0,
                                                    NULL, NULL, NULL, NULL);

              if (dist >= 0.0 &&
                  dist <  MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  ret     = path;
                }
            }
        }
    }

  g_list_free (all_path);

  return ret;
}

static GimpGuide *
gimp_image_pick_guide_internal (GimpImage           *image,
                                gdouble              x,
                                gdouble              y,
                                gdouble              epsilon_x,
                                gdouble              epsilon_y,
                                GimpOrientationType  orientation)
{
  GList     *list;
  GimpGuide *ret     = NULL;
  gdouble    mindist = G_MAXDOUBLE;

  for (list = GIMP_IMAGE_GET_PRIVATE (image)->guides;
       list;
       list = g_list_next (list))
    {
      GimpGuide *guide    = list->data;
      gint       position = gimp_guide_get_position (guide);
      gdouble    dist;

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          if (orientation == GIMP_ORIENTATION_HORIZONTAL ||
              orientation == GIMP_ORIENTATION_UNKNOWN)
            {
              dist = ABS (position - y);
              if (dist < MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  ret = guide;
                }
            }
          break;

        /* mindist always is in vertical resolution to make it comparable */
        case GIMP_ORIENTATION_VERTICAL:
          if (orientation == GIMP_ORIENTATION_VERTICAL ||
              orientation == GIMP_ORIENTATION_UNKNOWN)
            {
              dist = ABS (position - x);
              if (dist < MIN (epsilon_x, mindist / epsilon_y * epsilon_x))
                {
                  mindist = dist * epsilon_y / epsilon_x;
                  ret = guide;
                }
            }
          break;

        default:
          continue;
        }
    }

  return ret;
}

GimpGuide *
gimp_image_pick_guide (GimpImage *image,
                       gdouble    x,
                       gdouble    y,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  return gimp_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                         GIMP_ORIENTATION_UNKNOWN);
}

GList *
gimp_image_pick_guides (GimpImage *image,
                        gdouble    x,
                        gdouble    y,
                        gdouble    epsilon_x,
                        gdouble    epsilon_y)
{
  GimpGuide *guide;
  GList     *result = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  guide = gimp_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          GIMP_ORIENTATION_HORIZONTAL);

  if (guide)
    result = g_list_append (result, guide);

  guide = gimp_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          GIMP_ORIENTATION_VERTICAL);

  if (guide)
    result = g_list_append (result, guide);

  return result;
}

GimpSamplePoint *
gimp_image_pick_sample_point (GimpImage *image,
                              gdouble    x,
                              gdouble    y,
                              gdouble    epsilon_x,
                              gdouble    epsilon_y)
{
  GList           *list;
  GimpSamplePoint *ret     = NULL;
  gdouble          mindist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= gimp_image_get_width  (image) ||
      y < 0 || y >= gimp_image_get_height (image))
    {
      return NULL;
    }

  for (list = GIMP_IMAGE_GET_PRIVATE (image)->sample_points;
       list;
       list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;
      gint             sp_x;
      gint             sp_y;
      gdouble          dist;

      gimp_sample_point_get_position (sample_point, &sp_x, &sp_y);

      if (sp_x < 0 || sp_y < 0)
        continue;

      dist = hypot ((sp_x + 0.5) - x,
                    (sp_y + 0.5) - y);
      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          ret = sample_point;
        }
    }

  return ret;
}
