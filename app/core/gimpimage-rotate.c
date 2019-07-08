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

#include "core-types.h"

#include "vectors/gimpvectors.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-rotate.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimplayer.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"
#include "gimpsamplepoint.h"


static void  gimp_image_rotate_item_offset   (GimpImage        *image,
                                              GimpRotationType  rotate_type,
                                              GimpItem         *item,
                                              gint              off_x,
                                              gint              off_y);
static void  gimp_image_rotate_guides        (GimpImage        *image,
                                              GimpRotationType  rotate_type);
static void  gimp_image_rotate_sample_points (GimpImage        *image,
                                              GimpRotationType  rotate_type);


void
gimp_image_rotate (GimpImage        *image,
                   GimpContext      *context,
                   GimpRotationType  rotate_type,
                   GimpProgress     *progress)
{
  GimpObjectQueue *queue;
  GimpItem        *item;
  GList           *list;
  gdouble          center_x;
  gdouble          center_y;
  gint             new_image_width;
  gint             new_image_height;
  gint             previous_image_width;
  gint             previous_image_height;
  gint             offset_x;
  gint             offset_y;
  gboolean         size_changed;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  previous_image_width  = gimp_image_get_width  (image);
  previous_image_height = gimp_image_get_height (image);

  center_x              = previous_image_width  / 2.0;
  center_y              = previous_image_height / 2.0;

  /*  Resize the image (if needed)  */
  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
    case GIMP_ROTATE_270:
      new_image_width  = gimp_image_get_height (image);
      new_image_height = gimp_image_get_width  (image);
      size_changed     = TRUE;
      offset_x         = (gimp_image_get_width  (image) - new_image_width)  / 2;
      offset_y         = (gimp_image_get_height (image) - new_image_height) / 2;
      break;

    case GIMP_ROTATE_180:
      new_image_width  = gimp_image_get_width  (image);
      new_image_height = gimp_image_get_height (image);
      size_changed     = FALSE;
      offset_x         = 0;
      offset_y         = 0;
      break;

    default:
      g_return_if_reached ();
      return;
    }

  gimp_set_busy (image->gimp);

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  gimp_object_queue_push_container (queue, gimp_image_get_layers (image));
  gimp_object_queue_push (queue, gimp_image_get_mask (image));
  gimp_object_queue_push_container (queue, gimp_image_get_channels (image));
  gimp_object_queue_push_container (queue, gimp_image_get_vectors (image));

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ROTATE, NULL);

  /*  Rotate all layers, channels (including selection mask), and vectors  */
  while ((item = gimp_object_queue_pop (queue)))
    {
      gint off_x;
      gint off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      gimp_item_rotate (item, context, rotate_type, center_x, center_y, FALSE);

      if (GIMP_IS_LAYER (item))
        {
          gimp_image_rotate_item_offset (image, rotate_type, item, off_x, off_y);
        }
      else
        {
          gimp_item_set_offset (item, 0, 0);

          if (GIMP_IS_VECTORS (item))
            {
              gimp_item_set_size (item, new_image_width, new_image_height);

              gimp_item_translate (item,
                                   (new_image_width  - gimp_image_get_width  (image)) / 2,
                                   (new_image_height - gimp_image_get_height (image)) / 2,
                                   FALSE);
            }
        }

      gimp_progress_set_value (progress, 1.0);
    }

  /*  Rotate all Guides  */
  gimp_image_rotate_guides (image, rotate_type);

  /*  Rotate all sample points  */
  gimp_image_rotate_sample_points (image, rotate_type);

  /*  Resize the image (if needed)  */
  if (size_changed)
    {
      gdouble xres;
      gdouble yres;

      gimp_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       new_image_width,
                                       new_image_height);

      g_object_set (image,
                    "width",  new_image_width,
                    "height", new_image_height,
                    NULL);

      gimp_image_get_resolution (image, &xres, &yres);

      if (xres != yres)
        gimp_image_set_resolution (image, yres, xres);
    }

  /*  Notify guide movements  */
  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      gimp_image_guide_moved (image, list->data);
    }

  /*  Notify sample point movements  */
  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      gimp_image_sample_point_moved (image, list->data);
    }

  gimp_image_undo_group_end (image);

  g_object_unref (queue);

  if (size_changed)
    gimp_image_size_changed_detailed (image,
                                      -offset_x,
                                      -offset_y,
                                      previous_image_width,
                                      previous_image_height);

  g_object_thaw_notify (G_OBJECT (image));

  gimp_unset_busy (image->gimp);
}


static void
gimp_image_rotate_item_offset (GimpImage        *image,
                               GimpRotationType  rotate_type,
                               GimpItem         *item,
                               gint              off_x,
                               gint              off_y)
{
  gint x = 0;
  gint y = 0;

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      x = gimp_image_get_height (image) - off_y - gimp_item_get_width (item);
      y = off_x;
      break;

    case GIMP_ROTATE_270:
      x = off_y;
      y = gimp_image_get_width (image) - off_x - gimp_item_get_height (item);
      break;

    case GIMP_ROTATE_180:
      return;

    default:
      g_return_if_reached ();
    }

  gimp_item_get_offset (item, &off_x, &off_y);

  x -= off_x;
  y -= off_y;

  if (x || y)
    gimp_item_translate (item, x, y, FALSE);
}

static void
gimp_image_rotate_guides (GimpImage        *image,
                          GimpRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all Guides  */
  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      GimpGuide           *guide       = list->data;
      GimpOrientationType  orientation = gimp_guide_get_orientation (guide);
      gint                 position    = gimp_guide_get_position (guide);

      switch (rotate_type)
        {
        case GIMP_ROTATE_90:
          switch (orientation)
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              gimp_image_undo_push_guide (image, NULL, guide);
              gimp_guide_set_orientation (guide, GIMP_ORIENTATION_VERTICAL);
              gimp_guide_set_position (guide,
                                       gimp_image_get_height (image) - position);
              break;

            case GIMP_ORIENTATION_VERTICAL:
              gimp_image_undo_push_guide (image, NULL, guide);
              gimp_guide_set_orientation (guide, GIMP_ORIENTATION_HORIZONTAL);
              break;

            default:
              break;
            }
          break;

        case GIMP_ROTATE_180:
          switch (orientation)
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              gimp_image_move_guide (image, guide,
                                     gimp_image_get_height (image) - position,
                                     TRUE);
              break;

            case GIMP_ORIENTATION_VERTICAL:
              gimp_image_move_guide (image, guide,
                                     gimp_image_get_width (image) - position,
                                     TRUE);
              break;

            default:
              break;
            }
          break;

        case GIMP_ROTATE_270:
          switch (orientation)
            {
            case GIMP_ORIENTATION_HORIZONTAL:
              gimp_image_undo_push_guide (image, NULL, guide);
              gimp_guide_set_orientation (guide, GIMP_ORIENTATION_VERTICAL);
              break;

            case GIMP_ORIENTATION_VERTICAL:
              gimp_image_undo_push_guide (image, NULL, guide);
              gimp_guide_set_orientation (guide, GIMP_ORIENTATION_HORIZONTAL);
              gimp_guide_set_position (guide,
                                       gimp_image_get_width (image) - position);
              break;

            default:
              break;
            }
          break;
        }
    }
}


static void
gimp_image_rotate_sample_points (GimpImage        *image,
                                 GimpRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all sample points  */
  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;
      gint             old_x;
      gint             old_y;

      gimp_image_undo_push_sample_point (image, NULL, sample_point);

      gimp_sample_point_get_position (sample_point, &old_x, &old_y);

      switch (rotate_type)
        {
        case GIMP_ROTATE_90:
          gimp_sample_point_set_position (sample_point,
                                          gimp_image_get_height (image) - old_y,
                                          old_x);
          break;

        case GIMP_ROTATE_180:
          gimp_sample_point_set_position (sample_point,
                                          gimp_image_get_width  (image) - old_x,
                                          gimp_image_get_height (image) - old_y);
          break;

        case GIMP_ROTATE_270:
          gimp_sample_point_set_position (sample_point,
                                          old_y,
                                          gimp_image_get_width (image) - old_x);
          break;
        }
    }
}
