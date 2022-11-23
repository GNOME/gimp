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
#include <gexiv2/gexiv2.h>

#include "core-types.h"

#include "config/ligmadialogconfig.h"

#include "vectors/ligmavectors.h"

#include "ligma.h"
#include "ligmacontainer.h"
#include "ligmacontext.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-flip.h"
#include "ligmaimage-metadata.h"
#include "ligmaimage-rotate.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmalayer.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"
#include "ligmasamplepoint.h"


static void  ligma_image_rotate_item_offset   (LigmaImage         *image,
                                              LigmaRotationType   rotate_type,
                                              LigmaItem          *item,
                                              gint               off_x,
                                              gint               off_y);
static void  ligma_image_rotate_guides        (LigmaImage         *image,
                                              LigmaRotationType   rotate_type);
static void  ligma_image_rotate_sample_points (LigmaImage         *image,
                                              LigmaRotationType   rotate_type);

static void  ligma_image_metadata_rotate      (LigmaImage         *image,
                                              LigmaContext       *context,
                                              GExiv2Orientation  orientation,
                                              LigmaProgress      *progress);


/* Public Functions */

void
ligma_image_rotate (LigmaImage        *image,
                   LigmaContext      *context,
                   LigmaRotationType  rotate_type,
                   LigmaProgress     *progress)
{
  LigmaObjectQueue *queue;
  LigmaItem        *item;
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

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  previous_image_width  = ligma_image_get_width  (image);
  previous_image_height = ligma_image_get_height (image);

  center_x              = previous_image_width  / 2.0;
  center_y              = previous_image_height / 2.0;

  /*  Resize the image (if needed)  */
  switch (rotate_type)
    {
    case LIGMA_ROTATE_90:
    case LIGMA_ROTATE_270:
      new_image_width  = ligma_image_get_height (image);
      new_image_height = ligma_image_get_width  (image);
      size_changed     = TRUE;
      offset_x         = (ligma_image_get_width  (image) - new_image_width)  / 2;
      offset_y         = (ligma_image_get_height (image) - new_image_height) / 2;
      break;

    case LIGMA_ROTATE_180:
      new_image_width  = ligma_image_get_width  (image);
      new_image_height = ligma_image_get_height (image);
      size_changed     = FALSE;
      offset_x         = 0;
      offset_y         = 0;
      break;

    default:
      g_return_if_reached ();
      return;
    }

  ligma_set_busy (image->ligma);

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  ligma_object_queue_push_container (queue, ligma_image_get_layers (image));
  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));
  ligma_object_queue_push_container (queue, ligma_image_get_vectors (image));

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ROTATE, NULL);

  /*  Rotate all layers, channels (including selection mask), and vectors  */
  while ((item = ligma_object_queue_pop (queue)))
    {
      gint off_x;
      gint off_y;

      ligma_item_get_offset (item, &off_x, &off_y);

      ligma_item_rotate (item, context, rotate_type, center_x, center_y, FALSE);

      if (LIGMA_IS_LAYER (item))
        {
          ligma_image_rotate_item_offset (image, rotate_type, item, off_x, off_y);
        }
      else
        {
          ligma_item_set_offset (item, 0, 0);

          if (LIGMA_IS_VECTORS (item))
            {
              ligma_item_set_size (item, new_image_width, new_image_height);

              ligma_item_translate (item,
                                   (new_image_width  - ligma_image_get_width  (image)) / 2,
                                   (new_image_height - ligma_image_get_height (image)) / 2,
                                   FALSE);
            }
        }

      ligma_progress_set_value (progress, 1.0);
    }

  /*  Rotate all Guides  */
  ligma_image_rotate_guides (image, rotate_type);

  /*  Rotate all sample points  */
  ligma_image_rotate_sample_points (image, rotate_type);

  /*  Resize the image (if needed)  */
  if (size_changed)
    {
      gdouble xres;
      gdouble yres;

      ligma_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       new_image_width,
                                       new_image_height);

      g_object_set (image,
                    "width",  new_image_width,
                    "height", new_image_height,
                    NULL);

      ligma_image_get_resolution (image, &xres, &yres);

      if (xres != yres)
        ligma_image_set_resolution (image, yres, xres);
    }

  /*  Notify guide movements  */
  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      ligma_image_guide_moved (image, list->data);
    }

  /*  Notify sample point movements  */
  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      ligma_image_sample_point_moved (image, list->data);
    }

  ligma_image_undo_group_end (image);

  g_object_unref (queue);

  if (size_changed)
    ligma_image_size_changed_detailed (image,
                                      -offset_x,
                                      -offset_y,
                                      previous_image_width,
                                      previous_image_height);

  g_object_thaw_notify (G_OBJECT (image));

  ligma_unset_busy (image->ligma);
}

void
ligma_image_import_rotation_metadata (LigmaImage    *image,
                                     LigmaContext  *context,
                                     LigmaProgress *progress,
                                     gboolean      interactive)
{
  LigmaMetadata *metadata;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      LigmaMetadataRotationPolicy policy;

      policy = LIGMA_DIALOG_CONFIG (image->ligma->config)->metadata_rotation_policy;
      if (policy == LIGMA_METADATA_ROTATION_POLICY_ASK)
        {
          if (interactive)
            {
              gboolean dont_ask = FALSE;

              policy = ligma_query_rotation_policy (image->ligma, image,
                                                   context, &dont_ask);

              if (dont_ask)
                {
                  g_object_set (G_OBJECT (image->ligma->config),
                                "metadata-rotation-policy", policy,
                                NULL);
                }
            }
          else
            {
              policy = LIGMA_METADATA_ROTATION_POLICY_ROTATE;
            }
        }

      if (policy == LIGMA_METADATA_ROTATION_POLICY_ROTATE)
        ligma_image_metadata_rotate (image, context,
                                    gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata)),
                                    progress);

      gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                       GEXIV2_ORIENTATION_NORMAL);
    }
}


/* Private Functions */

static void
ligma_image_rotate_item_offset (LigmaImage        *image,
                               LigmaRotationType  rotate_type,
                               LigmaItem         *item,
                               gint              off_x,
                               gint              off_y)
{
  gint x = 0;
  gint y = 0;

  switch (rotate_type)
    {
    case LIGMA_ROTATE_90:
      x = ligma_image_get_height (image) - off_y - ligma_item_get_width (item);
      y = off_x;
      break;

    case LIGMA_ROTATE_270:
      x = off_y;
      y = ligma_image_get_width (image) - off_x - ligma_item_get_height (item);
      break;

    case LIGMA_ROTATE_180:
      return;

    default:
      g_return_if_reached ();
    }

  ligma_item_get_offset (item, &off_x, &off_y);

  x -= off_x;
  y -= off_y;

  if (x || y)
    ligma_item_translate (item, x, y, FALSE);
}

static void
ligma_image_rotate_guides (LigmaImage        *image,
                          LigmaRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all Guides  */
  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      LigmaGuide           *guide       = list->data;
      LigmaOrientationType  orientation = ligma_guide_get_orientation (guide);
      gint                 position    = ligma_guide_get_position (guide);

      switch (rotate_type)
        {
        case LIGMA_ROTATE_90:
          switch (orientation)
            {
            case LIGMA_ORIENTATION_HORIZONTAL:
              ligma_image_undo_push_guide (image, NULL, guide);
              ligma_guide_set_orientation (guide, LIGMA_ORIENTATION_VERTICAL);
              ligma_guide_set_position (guide,
                                       ligma_image_get_height (image) - position);
              break;

            case LIGMA_ORIENTATION_VERTICAL:
              ligma_image_undo_push_guide (image, NULL, guide);
              ligma_guide_set_orientation (guide, LIGMA_ORIENTATION_HORIZONTAL);
              break;

            default:
              break;
            }
          break;

        case LIGMA_ROTATE_180:
          switch (orientation)
            {
            case LIGMA_ORIENTATION_HORIZONTAL:
              ligma_image_move_guide (image, guide,
                                     ligma_image_get_height (image) - position,
                                     TRUE);
              break;

            case LIGMA_ORIENTATION_VERTICAL:
              ligma_image_move_guide (image, guide,
                                     ligma_image_get_width (image) - position,
                                     TRUE);
              break;

            default:
              break;
            }
          break;

        case LIGMA_ROTATE_270:
          switch (orientation)
            {
            case LIGMA_ORIENTATION_HORIZONTAL:
              ligma_image_undo_push_guide (image, NULL, guide);
              ligma_guide_set_orientation (guide, LIGMA_ORIENTATION_VERTICAL);
              break;

            case LIGMA_ORIENTATION_VERTICAL:
              ligma_image_undo_push_guide (image, NULL, guide);
              ligma_guide_set_orientation (guide, LIGMA_ORIENTATION_HORIZONTAL);
              ligma_guide_set_position (guide,
                                       ligma_image_get_width (image) - position);
              break;

            default:
              break;
            }
          break;
        }
    }
}


static void
ligma_image_rotate_sample_points (LigmaImage        *image,
                                 LigmaRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all sample points  */
  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      LigmaSamplePoint *sample_point = list->data;
      gint             old_x;
      gint             old_y;

      ligma_image_undo_push_sample_point (image, NULL, sample_point);

      ligma_sample_point_get_position (sample_point, &old_x, &old_y);

      switch (rotate_type)
        {
        case LIGMA_ROTATE_90:
          ligma_sample_point_set_position (sample_point,
                                          ligma_image_get_height (image) - old_y,
                                          old_x);
          break;

        case LIGMA_ROTATE_180:
          ligma_sample_point_set_position (sample_point,
                                          ligma_image_get_width  (image) - old_x,
                                          ligma_image_get_height (image) - old_y);
          break;

        case LIGMA_ROTATE_270:
          ligma_sample_point_set_position (sample_point,
                                          old_y,
                                          ligma_image_get_width (image) - old_x);
          break;
        }
    }
}

static void
ligma_image_metadata_rotate (LigmaImage         *image,
                            LigmaContext       *context,
                            GExiv2Orientation  orientation,
                            LigmaProgress      *progress)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      ligma_image_flip (image, context, LIGMA_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      ligma_image_rotate (image, context, LIGMA_ROTATE_180, progress);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      ligma_image_flip (image, context, LIGMA_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      ligma_image_rotate (image, context, LIGMA_ROTATE_90, progress);
      ligma_image_flip (image, context, LIGMA_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      ligma_image_rotate (image, context, LIGMA_ROTATE_90, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      ligma_image_rotate (image, context, LIGMA_ROTATE_90, progress);
      ligma_image_flip (image, context, LIGMA_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      ligma_image_rotate (image, context, LIGMA_ROTATE_270, progress);
      break;

    default: /* shouldn't happen */
      break;
    }
}
