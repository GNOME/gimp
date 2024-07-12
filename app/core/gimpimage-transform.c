/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-transform.c
 * Copyright (C) 2019 Ell
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

#include "vectors/gimppath.h"

#include "gimp.h"
#include "gimp-transform-resize.h"
#include "gimp-transform-utils.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-transform.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"
#include "gimpsamplepoint.h"


#define EPSILON 1e-6


/*  local function prototypes  */

static void    gimp_image_transform_guides        (GimpImage           *image,
                                                   const GimpMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);
static void    gimp_image_transform_sample_points (GimpImage           *image,
                                                   const GimpMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);


/*  private functions  */

static void
gimp_image_transform_guides (GimpImage           *image,
                             const GimpMatrix3   *matrix,
                             const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = gimp_image_get_guides (image); iter;)
    {
      GimpGuide           *guide           = iter->data;
      GimpOrientationType  old_orientation = gimp_guide_get_orientation (guide);
      gint                 old_position    = gimp_guide_get_position (guide);
      GimpOrientationType  new_orientation;
      gint                 new_position;
      GimpVector2          vertices[2];
      gint                 n_vertices;
      GimpVector2          diff;

      iter = g_list_next (iter);

      switch (old_orientation)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          vertices[0].x = old_bounds->x;
          vertices[0].y = old_bounds->y + old_position;

          vertices[1].x = old_bounds->x + old_bounds->width / 2.0;
          vertices[1].y = old_bounds->y + old_position;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          vertices[0].x = old_bounds->x + old_position;
          vertices[0].y = old_bounds->y;

          vertices[1].x = old_bounds->x + old_position;
          vertices[1].y = old_bounds->y + old_bounds->height / 2.0;
          break;

        case GIMP_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      gimp_transform_polygon (matrix,
                              vertices, 2, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 2)
        {
          gimp_image_remove_guide (image, guide, TRUE);

          continue;
        }

      gimp_vector2_sub (&diff, &vertices[1], &vertices[0]);

      if (gimp_vector2_length (&diff) <= EPSILON)
        {
          gimp_image_remove_guide (image, guide, TRUE);

          continue;
        }

      if (fabs (diff.x) >= fabs (diff.y))
        {
          new_orientation = GIMP_ORIENTATION_HORIZONTAL;
          new_position    = SIGNED_ROUND (vertices[1].y);

          if (new_position < 0 || new_position > gimp_image_get_height (image))
            {
              gimp_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }
      else
        {
          new_orientation = GIMP_ORIENTATION_VERTICAL;
          new_position    = SIGNED_ROUND (vertices[1].x);

          if (new_position < 0 || new_position > gimp_image_get_width (image))
            {
              gimp_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }

      if (new_orientation != old_orientation ||
          new_position    != old_position)
        {
          gimp_image_undo_push_guide (image, NULL, guide);

          gimp_guide_set_orientation (guide, new_orientation);
          gimp_guide_set_position    (guide, new_position);

          gimp_image_guide_moved (image, guide);
        }
    }
}

static void
gimp_image_transform_sample_points (GimpImage           *image,
                                    const GimpMatrix3   *matrix,
                                    const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = gimp_image_get_sample_points (image); iter;)
    {
      GimpSamplePoint     *sample_point = iter->data;
      gint                 old_x;
      gint                 old_y;
      gint                 new_x;
      gint                 new_y;
      GimpVector2          vertices[1];
      gint                 n_vertices;

      iter = g_list_next (iter);

      gimp_sample_point_get_position (sample_point, &old_x, &old_y);

      vertices[0].x = old_x;
      vertices[0].y = old_y;

      gimp_transform_polygon (matrix,
                              vertices, 1, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 1)
        {
          gimp_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      new_x = SIGNED_ROUND (vertices[0].x);
      new_y = SIGNED_ROUND (vertices[0].y);

      if (new_x < 0 || new_x >= gimp_image_get_width  (image) ||
          new_y < 0 || new_y >= gimp_image_get_height (image))
        {
          gimp_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      if (new_x != old_x || new_y != old_y)
        gimp_image_move_sample_point (image, sample_point, new_x, new_y, TRUE);
    }
}


/*  public functions  */

void
gimp_image_transform (GimpImage              *image,
                      GimpContext            *context,
                      const GimpMatrix3      *matrix,
                      GimpTransformDirection  direction,
                      GimpInterpolationType   interpolation_type,
                      GimpTransformResize     clip_result,
                      GimpProgress           *progress)
{
  GimpObjectQueue *queue;
  GimpItem        *item;
  GimpMatrix3      transform;
  GeglRectangle    old_bounds;
  GeglRectangle    new_bounds;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_set_busy (image->gimp);

  old_bounds.x      = 0;
  old_bounds.y      = 0;
  old_bounds.width  = gimp_image_get_width  (image);
  old_bounds.height = gimp_image_get_height (image);

  transform = *matrix;

  if (direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&transform);

  gimp_transform_resize_boundary (&transform, clip_result,

                                  old_bounds.x,
                                  old_bounds.y,
                                  old_bounds.x + old_bounds.width,
                                  old_bounds.y + old_bounds.height,

                                  &new_bounds.x,
                                  &new_bounds.y,
                                  &new_bounds.width,
                                  &new_bounds.height);

  new_bounds.width  -= new_bounds.x;
  new_bounds.height -= new_bounds.y;

  gimp_matrix3_translate (&transform,
                          old_bounds.x - new_bounds.x,
                          old_bounds.y - new_bounds.y);

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  gimp_object_queue_push_container (queue, gimp_image_get_layers (image));
  gimp_object_queue_push (queue, gimp_image_get_mask (image));
  gimp_object_queue_push_container (queue, gimp_image_get_channels (image));
  gimp_object_queue_push_container (queue, gimp_image_get_paths (image));

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_TRANSFORM, NULL);

  /*  Transform all layers, channels (including selection mask), and path  */
  while ((item = gimp_object_queue_pop (queue)))
    {
      GimpTransformResize clip = GIMP_TRANSFORM_RESIZE_ADJUST;

      if (GIMP_IS_CHANNEL (item))
        clip = clip_result;

      gimp_item_transform (item,
                           context,
                           &transform, direction,
                           interpolation_type, clip,
                           progress);

      if (GIMP_IS_PATH (item))
        gimp_item_set_size (item, new_bounds.width, new_bounds.height);
    }

  /*  Resize the image (if needed)  */
  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      gimp_image_undo_push_image_size (image,
                                       NULL,
                                       new_bounds.x,
                                       new_bounds.y,
                                       new_bounds.width,
                                       new_bounds.height);

      g_object_set (image,
                    "width",  new_bounds.width,
                    "height", new_bounds.height,
                    NULL);
    }

  /*  Transform all Guides  */
  gimp_image_transform_guides (image, &transform, &old_bounds);

  /*  Transform all sample points  */
  gimp_image_transform_sample_points (image, &transform, &old_bounds);

  gimp_image_undo_group_end (image);

  g_object_unref (queue);

  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      gimp_image_size_changed_detailed (image,
                                        old_bounds.x - new_bounds.x,
                                        old_bounds.y - new_bounds.y,
                                        old_bounds.width,
                                        old_bounds.height);
    }

  g_object_thaw_notify (G_OBJECT (image));

  gimp_unset_busy (image->gimp);
}
