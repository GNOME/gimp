/* The GIMP -- an image manipulation program
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
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-resize.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


void
gimp_image_resize (GimpImage    *gimage,
                   GimpContext  *context,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y,
                   GimpProgress *progress)
{
  GList   *list;
  gdouble  progress_max;
  gdouble  progress_current = 1.0;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_set_busy (gimage->gimp);

  progress_max = (gimage->channels->num_children +
                  gimage->layers->num_children   +
                  gimage->vectors->num_children  +
                  1 /* selection */);

  g_object_freeze_notify (G_OBJECT (gimage));

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_RESIZE,
                               _("Resize Image"));

  /*  Push the image size to the stack  */
  gimp_image_undo_push_image_size (gimage, NULL);

  /*  Set the new width and height  */
  g_object_set (gimage,
                "width",  new_width,
                "height", new_height,
                NULL);

  /*  Resize all channels  */
  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, context,
                        new_width, new_height, offset_x, offset_y);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Resize all vectors  */
  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, context,
                        new_width, new_height, offset_x, offset_y);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Don't forget the selection mask!  */
  gimp_item_resize (GIMP_ITEM (gimp_image_get_mask (gimage)), context,
                    new_width, new_height, offset_x, offset_y);

  if (progress)
    gimp_progress_set_value (progress, progress_current++ / progress_max);

  /*  Reposition all layers  */
  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_translate (item, offset_x, offset_y, TRUE);

      if (progress)
        gimp_progress_set_value (progress, progress_current++ / progress_max);
    }

  /*  Reposition or remove all guides  */
  list = gimage->guides;
  while (list)
    {
      GimpGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       new_position = guide->position;

      list = g_list_next (list);

      switch (guide->orientation)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          new_position += offset_y;
          if (new_position < 0 || new_position > new_height)
            remove_guide = TRUE;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          new_position += offset_x;
          if (new_position < 0 || new_position > new_width)
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        gimp_image_remove_guide (gimage, guide, TRUE);
      else if (new_position != guide->position)
        gimp_image_move_guide (gimage, guide, new_position, TRUE);
    }

  gimp_image_undo_group_end (gimage);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));
  g_object_thaw_notify (G_OBJECT (gimage));

  gimp_unset_busy (gimage->gimp);
}

void
gimp_image_resize_to_layers (GimpImage    *gimage,
                             GimpContext  *context,
                             GimpProgress *progress)
{
  gint   min_x, max_x, min_y, max_y;
  GList *list = GIMP_LIST (gimage->layers)->list;
  GimpItem *item;
  
  if (!list)
    return;

  /* figure out starting values */
  item = list->data;
  min_x = item->offset_x;
  min_y = item->offset_y;
  max_x = item->offset_x + item->width;
  max_y = item->offset_y + item->height;

  /*  Respect all layers  */
  for (list = g_list_next (list);
       list;
       list = g_list_next (list))
    {
      item = list->data;

      min_x = MIN (min_x, item->offset_x);
      min_y = MIN (min_y, item->offset_y);
      max_x = MAX (max_x, item->offset_x + item->width);
      max_y = MAX (max_y, item->offset_y + item->height);
    }

  gimp_image_resize (gimage, context,
                     max_x - min_x, max_y - min_y,
                     - min_x, - min_y,
                     progress);
}

