/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-flip.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimplist.h"
#include "gimpprogress.h"
#include "gimpsamplepoint.h"


void
gimp_image_flip (GimpImage           *image,
                 GimpContext         *context,
                 GimpOrientationType  flip_type,
                 GimpProgress        *progress)
{
  GimpItem *item;
  GList    *list;
  gdouble   axis;
  gdouble   progress_max;
  gdouble   progress_current = 1.0;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_set_busy (image->gimp);

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      axis = (gdouble) image->width / 2.0;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      axis = (gdouble) image->height / 2.0;
      break;

    default:
      g_warning ("%s: unknown flip_type", G_STRFUNC);
      return;
    }

  progress_max = (image->channels->num_children +
                  image->layers->num_children   +
                  image->vectors->num_children  +
                  1 /* selection */);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_FLIP, NULL);

  /*  Flip all channels  */
  for (list = GIMP_LIST (image->channels)->list;
       list;
       list = g_list_next (list))
    {
      item = (GimpItem *) list->data;

      gimp_item_flip (item, context, flip_type, axis, TRUE);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Flip all vectors  */
  for (list = GIMP_LIST (image->vectors)->list;
       list;
       list = g_list_next (list))
    {
      item = (GimpItem *) list->data;

      gimp_item_flip (item, context, flip_type, axis, FALSE);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Don't forget the selection mask!  */
  gimp_item_flip (GIMP_ITEM (gimp_image_get_mask (image)), context,
                  flip_type, axis, TRUE);

  if (progress)
    gimp_progress_set_value (progress, progress_current++ / progress_max);

  /*  Flip all layers  */
  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      item = (GimpItem *) list->data;

      gimp_item_flip (item, context, flip_type, axis, FALSE);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Flip all Guides  */
  for (list = image->guides; list; list = g_list_next (list))
    {
      GimpGuide *guide    = list->data;
      gint       position = gimp_guide_get_position (guide);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          if (flip_type == GIMP_ORIENTATION_VERTICAL)
            gimp_image_move_guide (image, guide,
                                   image->height - position, TRUE);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
            gimp_image_move_guide (image, guide,
                                   image->width - position, TRUE);
          break;

        default:
          break;
        }
    }

  /*  Flip all sample points  */
  for (list = image->sample_points; list; list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;

      if (flip_type == GIMP_ORIENTATION_VERTICAL)
        gimp_image_move_sample_point (image, sample_point,
                                      sample_point->x,
                                      image->height - sample_point->y,
                                      TRUE);

      if (flip_type == GIMP_ORIENTATION_HORIZONTAL)
        gimp_image_move_sample_point (image, sample_point,
                                      image->width - sample_point->x,
                                      sample_point->y,
                                      TRUE);
    }

  gimp_image_undo_group_end (image);

  gimp_unset_busy (image->gimp);
}
