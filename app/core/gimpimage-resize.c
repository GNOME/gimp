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
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-mask.h"
#include "gimpimage-projection.h"
#include "gimpimage-resize.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"

#include "libgimp/gimpintl.h"


void
gimp_image_resize (GimpImage *gimage, 
		   gint       new_width, 
		   gint       new_height,
		   gint       offset_x, 
		   gint       offset_y)
{
  GimpChannel *channel;
  GimpLayer   *layer;
  GimpLayer   *floating_layer;
  GList       *list;
  GList       *guide_list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (new_width > 0 && new_height > 0);

  gimp_set_busy (gimage->gimp);

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
      channel = (GimpChannel *) list->data;

      gimp_channel_resize (channel, new_width, new_height, offset_x, offset_y);
    }

  /*  Reposition or remove any guides  */
  guide_list = gimage->guides;
  while (guide_list)
    {
      GimpGuide *guide;

      guide = (GimpGuide *) guide_list->data;
      guide_list = g_list_next (guide_list);

      switch (guide->orientation)
	{
	case GIMP_ORIENTATION_HORIZONTAL:
	  gimp_image_undo_push_image_guide (gimage, NULL, guide);
	  guide->position += offset_y;
	  if (guide->position < 0 || guide->position > new_height)
	    gimp_image_delete_guide (gimage, guide);
	  break;

	case GIMP_ORIENTATION_VERTICAL:
	  gimp_image_undo_push_image_guide (gimage, NULL, guide);
	  guide->position += offset_x;
	  if (guide->position < 0 || guide->position > new_width)
	    gimp_image_delete_guide (gimage, guide);
	  break;

	default:
	  g_error ("Unknown guide orientation\n");
	}
    }

  /*  Don't forget the selection mask!  */
  gimp_channel_resize (gimage->selection_mask,
		       new_width, new_height, offset_x, offset_y);
  gimp_image_mask_invalidate (gimage);

  /*  Reposition all layers  */
  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      gimp_layer_translate (layer, offset_x, offset_y);
    }

  /*  Make sure the projection matches the gimage size  */
  gimp_image_projection_allocate (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  gimp_image_undo_group_end (gimage);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));

  gimp_image_mask_changed (gimage);

  gimp_unset_busy (gimage->gimp);
}
