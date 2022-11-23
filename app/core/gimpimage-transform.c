/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-transform.c
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

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "vectors/ligmavectors.h"

#include "ligma.h"
#include "ligma-transform-resize.h"
#include "ligma-transform-utils.h"
#include "ligmachannel.h"
#include "ligmacontext.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-transform.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"
#include "ligmasamplepoint.h"


#define EPSILON 1e-6


/*  local function prototypes  */

static void    ligma_image_transform_guides        (LigmaImage           *image,
                                                   const LigmaMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);
static void    ligma_image_transform_sample_points (LigmaImage           *image,
                                                   const LigmaMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);


/*  private functions  */

static void
ligma_image_transform_guides (LigmaImage           *image,
                             const LigmaMatrix3   *matrix,
                             const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = ligma_image_get_guides (image); iter;)
    {
      LigmaGuide           *guide           = iter->data;
      LigmaOrientationType  old_orientation = ligma_guide_get_orientation (guide);
      gint                 old_position    = ligma_guide_get_position (guide);
      LigmaOrientationType  new_orientation;
      gint                 new_position;
      LigmaVector2          vertices[2];
      gint                 n_vertices;
      LigmaVector2          diff;

      iter = g_list_next (iter);

      switch (old_orientation)
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          vertices[0].x = old_bounds->x;
          vertices[0].y = old_bounds->y + old_position;

          vertices[1].x = old_bounds->x + old_bounds->width / 2.0;
          vertices[1].y = old_bounds->y + old_position;
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          vertices[0].x = old_bounds->x + old_position;
          vertices[0].y = old_bounds->y;

          vertices[1].x = old_bounds->x + old_position;
          vertices[1].y = old_bounds->y + old_bounds->height / 2.0;
          break;

        case LIGMA_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      ligma_transform_polygon (matrix,
                              vertices, 2, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 2)
        {
          ligma_image_remove_guide (image, guide, TRUE);

          continue;
        }

      ligma_vector2_sub (&diff, &vertices[1], &vertices[0]);

      if (ligma_vector2_length (&diff) <= EPSILON)
        {
          ligma_image_remove_guide (image, guide, TRUE);

          continue;
        }

      if (fabs (diff.x) >= fabs (diff.y))
        {
          new_orientation = LIGMA_ORIENTATION_HORIZONTAL;
          new_position    = SIGNED_ROUND (vertices[1].y);

          if (new_position < 0 || new_position > ligma_image_get_height (image))
            {
              ligma_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }
      else
        {
          new_orientation = LIGMA_ORIENTATION_VERTICAL;
          new_position    = SIGNED_ROUND (vertices[1].x);

          if (new_position < 0 || new_position > ligma_image_get_width (image))
            {
              ligma_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }

      if (new_orientation != old_orientation ||
          new_position    != old_position)
        {
          ligma_image_undo_push_guide (image, NULL, guide);

          ligma_guide_set_orientation (guide, new_orientation);
          ligma_guide_set_position    (guide, new_position);

          ligma_image_guide_moved (image, guide);
        }
    }
}

static void
ligma_image_transform_sample_points (LigmaImage           *image,
                                    const LigmaMatrix3   *matrix,
                                    const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = ligma_image_get_sample_points (image); iter;)
    {
      LigmaSamplePoint     *sample_point = iter->data;
      gint                 old_x;
      gint                 old_y;
      gint                 new_x;
      gint                 new_y;
      LigmaVector2          vertices[1];
      gint                 n_vertices;

      iter = g_list_next (iter);

      ligma_sample_point_get_position (sample_point, &old_x, &old_y);

      vertices[0].x = old_x;
      vertices[0].y = old_y;

      ligma_transform_polygon (matrix,
                              vertices, 1, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 1)
        {
          ligma_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      new_x = SIGNED_ROUND (vertices[0].x);
      new_y = SIGNED_ROUND (vertices[0].y);

      if (new_x < 0 || new_x >= ligma_image_get_width  (image) ||
          new_y < 0 || new_y >= ligma_image_get_height (image))
        {
          ligma_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      if (new_x != old_x || new_y != old_y)
        ligma_image_move_sample_point (image, sample_point, new_x, new_y, TRUE);
    }
}


/*  public functions  */

void
ligma_image_transform (LigmaImage              *image,
                      LigmaContext            *context,
                      const LigmaMatrix3      *matrix,
                      LigmaTransformDirection  direction,
                      LigmaInterpolationType   interpolation_type,
                      LigmaTransformResize     clip_result,
                      LigmaProgress           *progress)
{
  LigmaObjectQueue *queue;
  LigmaItem        *item;
  LigmaMatrix3      transform;
  GeglRectangle    old_bounds;
  GeglRectangle    new_bounds;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  ligma_set_busy (image->ligma);

  old_bounds.x      = 0;
  old_bounds.y      = 0;
  old_bounds.width  = ligma_image_get_width  (image);
  old_bounds.height = ligma_image_get_height (image);

  transform = *matrix;

  if (direction == LIGMA_TRANSFORM_BACKWARD)
    ligma_matrix3_invert (&transform);

  ligma_transform_resize_boundary (&transform, clip_result,

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

  ligma_matrix3_translate (&transform,
                          old_bounds.x - new_bounds.x,
                          old_bounds.y - new_bounds.y);

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  ligma_object_queue_push_container (queue, ligma_image_get_layers (image));
  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));
  ligma_object_queue_push_container (queue, ligma_image_get_vectors (image));

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_TRANSFORM, NULL);

  /*  Transform all layers, channels (including selection mask), and vectors  */
  while ((item = ligma_object_queue_pop (queue)))
    {
      LigmaTransformResize clip = LIGMA_TRANSFORM_RESIZE_ADJUST;

      if (LIGMA_IS_CHANNEL (item))
        clip = clip_result;

      ligma_item_transform (item,
                           context,
                           &transform, direction,
                           interpolation_type, clip,
                           progress);

      if (LIGMA_IS_VECTORS (item))
        ligma_item_set_size (item, new_bounds.width, new_bounds.height);
    }

  /*  Resize the image (if needed)  */
  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      ligma_image_undo_push_image_size (image,
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
  ligma_image_transform_guides (image, &transform, &old_bounds);

  /*  Transform all sample points  */
  ligma_image_transform_sample_points (image, &transform, &old_bounds);

  ligma_image_undo_group_end (image);

  g_object_unref (queue);

  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      ligma_image_size_changed_detailed (image,
                                        old_bounds.x - new_bounds.x,
                                        old_bounds.y - new_bounds.y,
                                        old_bounds.width,
                                        old_bounds.height);
    }

  g_object_thaw_notify (G_OBJECT (image));

  ligma_unset_busy (image->ligma);
}
