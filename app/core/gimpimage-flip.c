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

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmacontainer.h"
#include "ligmacontext.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-flip.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"
#include "ligmasamplepoint.h"


/*  local function prototypes  */

static void    ligma_image_flip_guides        (LigmaImage           *image,
                                              LigmaOrientationType  flip_type,
                                              gdouble              axis);
static void    ligma_image_flip_sample_points (LigmaImage           *image,
                                              LigmaOrientationType  flip_type,
                                              gdouble              axis);


/*  private functions  */

static void
ligma_image_flip_guides (LigmaImage           *image,
                        LigmaOrientationType  flip_type,
                        gdouble              axis)
{
  gint   width  = ligma_image_get_width  (image);
  gint   height = ligma_image_get_height (image);
  GList *iter;

  for (iter = ligma_image_get_guides (image); iter;)
    {
      LigmaGuide *guide    = iter->data;
      gint       position = ligma_guide_get_position (guide);

      iter = g_list_next (iter);

      position = SIGNED_ROUND (2.0 * axis - position);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          if (flip_type == LIGMA_ORIENTATION_VERTICAL)
            {
              if (position >= 0 && position <= height)
                ligma_image_move_guide (image, guide, position, TRUE);
              else
                ligma_image_remove_guide (image, guide, TRUE);
            }
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          if (flip_type == LIGMA_ORIENTATION_HORIZONTAL)
            {
              if (position >= 0 && position <= width)
                ligma_image_move_guide (image, guide, position, TRUE);
              else
                ligma_image_remove_guide (image, guide, TRUE);
            }
          break;

        case LIGMA_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }
}

static void
ligma_image_flip_sample_points (LigmaImage           *image,
                               LigmaOrientationType  flip_type,
                               gdouble              axis)
{
  gint   width  = ligma_image_get_width  (image);
  gint   height = ligma_image_get_height (image);
  GList *iter;

  for (iter = ligma_image_get_sample_points (image); iter;)
    {
      LigmaSamplePoint *sample_point = iter->data;
      gint             x;
      gint             y;

      iter = g_list_next (iter);

      ligma_sample_point_get_position (sample_point, &x, &y);

      switch (flip_type)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          x = SIGNED_ROUND (2.0 * axis - x);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          y = SIGNED_ROUND (2.0 * axis - y);
          break;

        case LIGMA_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      if (x >= 0 && x < width &&
          y >= 0 && y < height)
        {
          ligma_image_move_sample_point (image, sample_point, x, y, TRUE);
        }
      else
        {
          ligma_image_remove_sample_point (image, sample_point, TRUE);
        }
    }
}


/*  public functions  */

void
ligma_image_flip (LigmaImage           *image,
                 LigmaContext         *context,
                 LigmaOrientationType  flip_type,
                 LigmaProgress        *progress)
{
  gdouble axis = 0.0;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  switch (flip_type)
    {
    case LIGMA_ORIENTATION_HORIZONTAL:
      axis = ligma_image_get_width (image) / 2.0;
      break;

    case LIGMA_ORIENTATION_VERTICAL:
      axis = ligma_image_get_height (image) / 2.0;
      break;

    case LIGMA_ORIENTATION_UNKNOWN:
      g_return_if_reached ();
    }

  ligma_image_flip_full (image, context, flip_type, axis,
                        LIGMA_TRANSFORM_RESIZE_CLIP, progress);
}

void
ligma_image_flip_full (LigmaImage           *image,
                      LigmaContext         *context,
                      LigmaOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result,
                      LigmaProgress        *progress)
{
  LigmaObjectQueue *queue;
  LigmaItem        *item;
  gint             width;
  gint             height;
  gint             offset_x = 0;
  gint             offset_y = 0;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  width  = ligma_image_get_width  (image);
  height = ligma_image_get_height (image);

  if (! clip_result)
    {
      switch (flip_type)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          offset_x = SIGNED_ROUND (2.0 * axis - width);
          axis     = width / 2.0;
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          offset_y = SIGNED_ROUND (2.0 * axis - height);
          axis     = height / 2.0;
          break;

        case LIGMA_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }

  ligma_set_busy (image->ligma);

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  ligma_object_queue_push_container (queue, ligma_image_get_layers (image));
  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));
  ligma_object_queue_push_container (queue, ligma_image_get_vectors (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_FLIP, NULL);

  /*  Flip all layers, channels (including selection mask), and vectors  */
  while ((item = ligma_object_queue_pop (queue)))
    {
      gboolean clip = FALSE;

      if (LIGMA_IS_CHANNEL (item))
        clip = clip_result;

      ligma_item_flip (item, context, flip_type, axis, clip);

      ligma_progress_set_value (progress, 1.0);
    }

  /*  Flip all Guides  */
  ligma_image_flip_guides (image, flip_type, axis);

  /*  Flip all sample points  */
  ligma_image_flip_sample_points (image, flip_type, axis);

  if (offset_x || offset_y)
    {
      ligma_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       width,
                                       height);
    }

  ligma_image_undo_group_end (image);

  g_object_unref (queue);

  if (offset_x || offset_y)
    {
      ligma_image_size_changed_detailed (image,
                                        -offset_x,
                                        -offset_y,
                                        width,
                                        height);
    }

  ligma_unset_busy (image->ligma);
}
