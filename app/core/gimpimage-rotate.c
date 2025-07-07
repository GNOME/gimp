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
#include <gexiv2/gexiv2.h>

#include "core-types.h"

#include "config/gimpdialogconfig.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-flip.h"
#include "gimpimage-metadata.h"
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

#include "path/gimppath.h"


static void  gimp_image_rotate_item_offset   (GimpImage         *image,
                                              GimpRotationType   rotate_type,
                                              GimpItem          *item,
                                              gint               off_x,
                                              gint               off_y);
static void  gimp_image_rotate_guides        (GimpImage         *image,
                                              GimpRotationType   rotate_type);
static void  gimp_image_rotate_sample_points (GimpImage         *image,
                                              GimpRotationType   rotate_type);

static void  gimp_image_metadata_rotate      (GimpImage         *image,
                                              GimpContext       *context,
                                              GExiv2Orientation  orientation,
                                              GimpProgress      *progress);


/* Public Functions */

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
    case GIMP_ROTATE_DEGREES90:
    case GIMP_ROTATE_DEGREES270:
      new_image_width  = gimp_image_get_height (image);
      new_image_height = gimp_image_get_width  (image);
      size_changed     = TRUE;
      offset_x         = (gimp_image_get_width  (image) - new_image_width)  / 2;
      offset_y         = (gimp_image_get_height (image) - new_image_height) / 2;
      break;

    case GIMP_ROTATE_DEGREES180:
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
  gimp_object_queue_push_container (queue, gimp_image_get_paths (image));

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ROTATE, NULL);

  /*  Rotate all layers, channels (including selection mask), and path  */
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

          if (GIMP_IS_PATH (item))
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

void
gimp_image_import_rotation_metadata (GimpImage    *image,
                                     GimpContext  *context,
                                     GimpProgress *progress,
                                     gboolean      interactive)
{
  GimpMetadata *metadata;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      GimpMetadataRotationPolicy policy;

      policy = GIMP_DIALOG_CONFIG (image->gimp->config)->metadata_rotation_policy;
      if (policy == GIMP_METADATA_ROTATION_POLICY_ASK)
        {
          if (interactive)
            {
              gboolean dont_ask = FALSE;

              policy = gimp_query_rotation_policy (image->gimp, image,
                                                   context, &dont_ask);

              if (dont_ask)
                {
                  g_object_set (G_OBJECT (image->gimp->config),
                                "metadata-rotation-policy", policy,
                                NULL);
                }
            }
          else
            {
              policy = GIMP_METADATA_ROTATION_POLICY_ROTATE;
            }
        }

      if (policy == GIMP_METADATA_ROTATION_POLICY_ROTATE)
        gimp_image_metadata_rotate (image, context,
                                    gexiv2_metadata_try_get_orientation (GEXIV2_METADATA (metadata), NULL),
                                    progress);

      gexiv2_metadata_try_set_orientation (GEXIV2_METADATA (metadata),
                                           GEXIV2_ORIENTATION_NORMAL,
                                           NULL);
    }
}

void
gimp_image_apply_metadata_orientation (GimpImage         *image,
                                       GimpContext       *context,
                                       GimpMetadata      *metadata,
                                       GimpProgress      *progress)
{
  gimp_image_metadata_rotate (image, context,
                              gexiv2_metadata_try_get_orientation (GEXIV2_METADATA (metadata), NULL),
                              progress);
}


/* Private Functions */

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
    case GIMP_ROTATE_DEGREES90:
      x = gimp_image_get_height (image) - off_y - gimp_item_get_width (item);
      y = off_x;
      break;

    case GIMP_ROTATE_DEGREES270:
      x = off_y;
      y = gimp_image_get_width (image) - off_x - gimp_item_get_height (item);
      break;

    case GIMP_ROTATE_DEGREES180:
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
        case GIMP_ROTATE_DEGREES90:
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

        case GIMP_ROTATE_DEGREES180:
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

        case GIMP_ROTATE_DEGREES270:
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
        case GIMP_ROTATE_DEGREES90:
          gimp_sample_point_set_position (sample_point,
                                          gimp_image_get_height (image) - old_y,
                                          old_x);
          break;

        case GIMP_ROTATE_DEGREES180:
          gimp_sample_point_set_position (sample_point,
                                          gimp_image_get_width  (image) - old_x,
                                          gimp_image_get_height (image) - old_y);
          break;

        case GIMP_ROTATE_DEGREES270:
          gimp_sample_point_set_position (sample_point,
                                          old_y,
                                          gimp_image_get_width (image) - old_x);
          break;
        }
    }
}

static void
gimp_image_metadata_rotate (GimpImage         *image,
                            GimpContext       *context,
                            GExiv2Orientation  orientation,
                            GimpProgress      *progress)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      gimp_image_flip (image, context, GIMP_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      gimp_image_rotate (image, context, GIMP_ROTATE_DEGREES180, progress);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      gimp_image_flip (image, context, GIMP_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      gimp_image_rotate (image, context, GIMP_ROTATE_DEGREES90, progress);
      gimp_image_flip (image, context, GIMP_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      gimp_image_rotate (image, context, GIMP_ROTATE_DEGREES90, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      gimp_image_rotate (image, context, GIMP_ROTATE_DEGREES90, progress);
      gimp_image_flip (image, context, GIMP_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      gimp_image_rotate (image, context, GIMP_ROTATE_DEGREES270, progress);
      break;

    default: /* shouldn't happen */
      break;
    }
}
