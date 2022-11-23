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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligmagrouplayer.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-pick-item.h"
#include "ligmaimage-private.h"
#include "ligmapickable.h"
#include "ligmasamplepoint.h"

#include "text/ligmatextlayer.h"

#include "vectors/ligmastroke.h"
#include "vectors/ligmavectors.h"


LigmaLayer *
ligma_image_pick_layer (LigmaImage *image,
                       gint       x,
                       gint       y,
                       LigmaLayer *previously_picked)
{
  GList *all_layers;
  GList *list;
  gint   off_x, off_y;
  gint   tries = 1;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  all_layers = ligma_image_get_layer_list (image);

  if (previously_picked)
    {
      ligma_item_get_offset (LIGMA_ITEM (previously_picked), &off_x, &off_y);
      if (ligma_pickable_get_opacity_at (LIGMA_PICKABLE (previously_picked),
                                        x - off_x, y - off_y) <= 0.25)
        previously_picked = NULL;
      else
        tries++;
    }

  while (tries)
    {
      for (list = all_layers; list; list = g_list_next (list))
        {
          LigmaLayer *layer = list->data;

          if (previously_picked)
            {
              /* Take the first layer with a pixel at given coordinates
               * after the previously picked one.
               */
              if (layer == previously_picked)
                previously_picked = NULL;
              continue;
            }

          ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);

          if (ligma_pickable_get_opacity_at (LIGMA_PICKABLE (layer),
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

LigmaLayer *
ligma_image_pick_layer_by_bounds (LigmaImage *image,
                                 gint       x,
                                 gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  all_layers = ligma_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      LigmaLayer *layer = list->data;

      if (ligma_item_is_visible (LIGMA_ITEM (layer)))
        {
          gint off_x, off_y;
          gint width, height;

          ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);
          width  = ligma_item_get_width  (LIGMA_ITEM (layer));
          height = ligma_item_get_height (LIGMA_ITEM (layer));

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

LigmaTextLayer *
ligma_image_pick_text_layer (LigmaImage *image,
                            gint       x,
                            gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  all_layers = ligma_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      LigmaLayer *layer = list->data;
      gint       off_x, off_y;

      ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);

      if (LIGMA_IS_TEXT_LAYER (layer) &&
          x >= off_x &&
          y >= off_y &&
          x <  off_x + ligma_item_get_width  (LIGMA_ITEM (layer)) &&
          y <  off_y + ligma_item_get_height (LIGMA_ITEM (layer)) &&
          ligma_item_is_visible (LIGMA_ITEM (layer)))
        {
          g_list_free (all_layers);

          return LIGMA_TEXT_LAYER (layer);
        }
      else if (ligma_pickable_get_opacity_at (LIGMA_PICKABLE (layer),
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

LigmaVectors *
ligma_image_pick_vectors (LigmaImage *image,
                         gdouble    x,
                         gdouble    y,
                         gdouble    epsilon_x,
                         gdouble    epsilon_y)
{
  LigmaVectors *ret = NULL;
  GList       *all_vectors;
  GList       *list;
  gdouble      mindist = G_MAXDOUBLE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  all_vectors = ligma_image_get_vectors_list (image);

  for (list = all_vectors; list; list = g_list_next (list))
    {
      LigmaVectors *vectors = list->data;

      if (ligma_item_is_visible (LIGMA_ITEM (vectors)))
        {
          LigmaStroke *stroke = NULL;
          LigmaCoords  coords = LIGMA_COORDS_DEFAULT_VALUES;

          while ((stroke = ligma_vectors_stroke_get_next (vectors, stroke)))
            {
              gdouble dist;

              coords.x = x;
              coords.y = y;

              dist = ligma_stroke_nearest_point_get (stroke, &coords, 1.0,
                                                    NULL, NULL, NULL, NULL);

              if (dist >= 0.0 &&
                  dist <  MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  ret     = vectors;
                }
            }
        }
    }

  g_list_free (all_vectors);

  return ret;
}

static LigmaGuide *
ligma_image_pick_guide_internal (LigmaImage           *image,
                                gdouble              x,
                                gdouble              y,
                                gdouble              epsilon_x,
                                gdouble              epsilon_y,
                                LigmaOrientationType  orientation)
{
  GList     *list;
  LigmaGuide *ret     = NULL;
  gdouble    mindist = G_MAXDOUBLE;

  for (list = LIGMA_IMAGE_GET_PRIVATE (image)->guides;
       list;
       list = g_list_next (list))
    {
      LigmaGuide *guide    = list->data;
      gint       position = ligma_guide_get_position (guide);
      gdouble    dist;

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          if (orientation == LIGMA_ORIENTATION_HORIZONTAL ||
              orientation == LIGMA_ORIENTATION_UNKNOWN)
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
        case LIGMA_ORIENTATION_VERTICAL:
          if (orientation == LIGMA_ORIENTATION_VERTICAL ||
              orientation == LIGMA_ORIENTATION_UNKNOWN)
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

LigmaGuide *
ligma_image_pick_guide (LigmaImage *image,
                       gdouble    x,
                       gdouble    y,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  return ligma_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                         LIGMA_ORIENTATION_UNKNOWN);
}

GList *
ligma_image_pick_guides (LigmaImage *image,
                        gdouble    x,
                        gdouble    y,
                        gdouble    epsilon_x,
                        gdouble    epsilon_y)
{
  LigmaGuide *guide;
  GList     *result = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  guide = ligma_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          LIGMA_ORIENTATION_HORIZONTAL);

  if (guide)
    result = g_list_append (result, guide);

  guide = ligma_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          LIGMA_ORIENTATION_VERTICAL);

  if (guide)
    result = g_list_append (result, guide);

  return result;
}

LigmaSamplePoint *
ligma_image_pick_sample_point (LigmaImage *image,
                              gdouble    x,
                              gdouble    y,
                              gdouble    epsilon_x,
                              gdouble    epsilon_y)
{
  GList           *list;
  LigmaSamplePoint *ret     = NULL;
  gdouble          mindist = G_MAXDOUBLE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= ligma_image_get_width  (image) ||
      y < 0 || y >= ligma_image_get_height (image))
    {
      return NULL;
    }

  for (list = LIGMA_IMAGE_GET_PRIVATE (image)->sample_points;
       list;
       list = g_list_next (list))
    {
      LigmaSamplePoint *sample_point = list->data;
      gint             sp_x;
      gint             sp_y;
      gdouble          dist;

      ligma_sample_point_get_position (sample_point, &sp_x, &sp_y);

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
