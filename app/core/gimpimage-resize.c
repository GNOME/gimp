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
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-resize.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"

#include "gimp-intl.h"


void
gimp_image_resize (GimpImage        *gimage,
		   gint              new_width,
		   gint              new_height,
		   gint              offset_x,
		   gint              offset_y,
                   GimpProgressFunc  progress_func,
                   gpointer          progress_data)
{
  GimpLayer *floating_layer;
  GList     *list;
  gint       progress_max;
  gint       progress_current = 1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (new_width > 0 && new_height > 0);

  gimp_set_busy (gimage->gimp);

  progress_max = (gimage->channels->num_children +
                  gimage->layers->num_children   +
                  gimage->vectors->num_children  +
                  1 /* selection */);

  /*  Get the floating layer if one exists  */
  floating_layer = gimp_image_floating_sel (gimage);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_RESIZE,
                               _("Resize Image"));

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  gimp_image_undo_push_image_size (gimage, NULL);

  /*  Set the new width and height  */
  gimage->width  = new_width;
  gimage->height = new_height;

  /*  Resize all channels  */
  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, new_width, new_height, offset_x, offset_y);

      if (progress_func)
        (* progress_func) (0, progress_max, progress_current++, progress_data);
    }

  /*  Resize all vectors  */
  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, new_width, new_height, offset_x, offset_y);

      if (progress_func)
        (* progress_func) (0, progress_max, progress_current++, progress_data);
    }

  /*  Don't forget the selection mask!  */
  gimp_item_resize (GIMP_ITEM (gimp_image_get_mask (gimage)),
                    new_width, new_height, offset_x, offset_y);

  if (progress_func)
    (* progress_func) (0, progress_max, progress_current++, progress_data);

  /*  Reposition all layers  */
  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_translate (item, offset_x, offset_y, TRUE);

      if (progress_func)
        (* progress_func) (0, progress_max, progress_current++, progress_data);
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

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  gimp_image_undo_group_end (gimage);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));

  gimp_unset_busy (gimage->gimp);
}
