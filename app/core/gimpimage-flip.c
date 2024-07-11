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

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-flip.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"
#include "gimpsamplepoint.h"


/*  local function prototypes  */

static void    gimp_image_flip_guides        (GimpImage           *image,
                                              GimpOrientationType  flip_type,
                                              gdouble              axis);
static void    gimp_image_flip_sample_points (GimpImage           *image,
                                              GimpOrientationType  flip_type,
                                              gdouble              axis);


/*  private functions  */

static void
gimp_image_flip_guides (GimpImage           *image,
                        GimpOrientationType  flip_type,
                        gdouble              axis)
{
  gint   width  = gimp_image_get_width  (image);
  gint   height = gimp_image_get_height (image);
  GList *iter;

  for (iter = gimp_image_get_guides (image); iter;)
    {
      GimpGuide *guide    = iter->data;
      gint       position = gimp_guide_get_position (guide);

      iter = g_list_next (iter);

      position = SIGNED_ROUND (2.0 * axis - position);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          if (flip_type == GIMP_ORIENTATION_VERTICAL)
            {
              if (position >= 0 && position <= height)
                gimp_image_move_guide (image, guide, position, TRUE);
              else
                gimp_image_remove_guide (image, guide, TRUE);
            }
          break;

        case GIMP_ORIENTATION_VERTICAL:
          if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
            {
              if (position >= 0 && position <= width)
                gimp_image_move_guide (image, guide, position, TRUE);
              else
                gimp_image_remove_guide (image, guide, TRUE);
            }
          break;

        case GIMP_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }
}

static void
gimp_image_flip_sample_points (GimpImage           *image,
                               GimpOrientationType  flip_type,
                               gdouble              axis)
{
  gint   width  = gimp_image_get_width  (image);
  gint   height = gimp_image_get_height (image);
  GList *iter;

  for (iter = gimp_image_get_sample_points (image); iter;)
    {
      GimpSamplePoint *sample_point = iter->data;
      gint             x;
      gint             y;

      iter = g_list_next (iter);

      gimp_sample_point_get_position (sample_point, &x, &y);

      switch (flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          x = SIGNED_ROUND (2.0 * axis - x);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          y = SIGNED_ROUND (2.0 * axis - y);
          break;

        case GIMP_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      if (x >= 0 && x < width &&
          y >= 0 && y < height)
        {
          gimp_image_move_sample_point (image, sample_point, x, y, TRUE);
        }
      else
        {
          gimp_image_remove_sample_point (image, sample_point, TRUE);
        }
    }
}


/*  public functions  */

void
gimp_image_flip (GimpImage           *image,
                 GimpContext         *context,
                 GimpOrientationType  flip_type,
                 GimpProgress        *progress)
{
  gdouble axis = 0.0;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      axis = gimp_image_get_width (image) / 2.0;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      axis = gimp_image_get_height (image) / 2.0;
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      g_return_if_reached ();
    }

  gimp_image_flip_full (image, context, flip_type, axis,
                        GIMP_TRANSFORM_RESIZE_CLIP, progress);
}

void
gimp_image_flip_full (GimpImage           *image,
                      GimpContext         *context,
                      GimpOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result,
                      GimpProgress        *progress)
{
  GimpObjectQueue *queue;
  GimpItem        *item;
  gint             width;
  gint             height;
  gint             offset_x = 0;
  gint             offset_y = 0;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  width  = gimp_image_get_width  (image);
  height = gimp_image_get_height (image);

  if (! clip_result)
    {
      switch (flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          offset_x = SIGNED_ROUND (2.0 * axis - width);
          axis     = width / 2.0;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          offset_y = SIGNED_ROUND (2.0 * axis - height);
          axis     = height / 2.0;
          break;

        case GIMP_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }

  gimp_set_busy (image->gimp);

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  gimp_object_queue_push_container (queue, gimp_image_get_layers (image));
  gimp_object_queue_push (queue, gimp_image_get_mask (image));
  gimp_object_queue_push_container (queue, gimp_image_get_channels (image));
  gimp_object_queue_push_container (queue, gimp_image_get_paths (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_FLIP, NULL);

  /*  Flip all layers, channels (including selection mask), and vectors  */
  while ((item = gimp_object_queue_pop (queue)))
    {
      gboolean clip = FALSE;

      if (GIMP_IS_CHANNEL (item))
        clip = clip_result;

      gimp_item_flip (item, context, flip_type, axis, clip);

      gimp_progress_set_value (progress, 1.0);
    }

  /*  Flip all Guides  */
  gimp_image_flip_guides (image, flip_type, axis);

  /*  Flip all sample points  */
  gimp_image_flip_sample_points (image, flip_type, axis);

  if (offset_x || offset_y)
    {
      gimp_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       width,
                                       height);
    }

  gimp_image_undo_group_end (image);

  g_object_unref (queue);

  if (offset_x || offset_y)
    {
      gimp_image_size_changed_detailed (image,
                                        -offset_x,
                                        -offset_y,
                                        width,
                                        height);
    }

  gimp_unset_busy (image->gimp);
}
