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
#include "libgimptool/gimptooltypes.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimpimage-projection.h"
#include "gimpimage-scale.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"

#include "undo.h"


void
gimp_image_scale (GimpImage             *gimage, 
		  gint                   new_width, 
		  gint                   new_height,
                  GimpInterpolationType  interpolation_type,
                  GimpProgressFunc       progress_func,
                  gpointer               progress_data)
{
  GimpChannel *channel;
  GimpLayer   *layer;
  GimpLayer   *floating_layer;
  GList       *list;
  GSList      *remove = NULL;
  GSList      *slist;
  GimpGuide   *guide;
  gint         old_width;
  gint         old_height;
  gdouble      img_scale_w = 1.0;
  gdouble      img_scale_h = 1.0;
  gint         num_channels;
  gint         num_layers;
  gint         progress_current = 1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (new_width > 0 && new_height > 0);

  gimp_set_busy (gimage->gimp);

  num_channels = gimage->channels->num_children;
  num_layers   = gimage->layers->num_children;

  /*  Get the floating layer if one exists  */
  floating_layer = gimp_image_floating_sel (gimage);

  undo_push_group_start (gimage, IMAGE_SCALE_UNDO_GROUP);

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  undo_push_image_size (gimage);

  /*  Set the new width and height  */

  old_width      = gimage->width;
  old_height     = gimage->height;
  gimage->width  = new_width;
  gimage->height = new_height;
  img_scale_w    = (gdouble) new_width  / (gdouble) old_width;
  img_scale_h    = (gdouble) new_height / (gdouble) old_height;
 
  /*  Scale all channels  */
  for (list = GIMP_LIST (gimage->channels)->list; 
       list; 
       list = g_list_next (list))
    {
      channel = (GimpChannel *) list->data;

      gimp_channel_scale (channel, new_width, new_height, interpolation_type);

      if (progress_func)
        {
          (* progress_func) (0, num_channels + num_layers,
                             progress_current++,
                             progress_data);
        }
    }

  /*  Don't forget the selection mask!  */
  /*  if (channel_is_empty(gimage->selection_mask))
        gimp_channel_resize(gimage->selection_mask, new_width, new_height, 0, 0)
      else
  */

  gimp_channel_scale (gimage->selection_mask, new_width, new_height,
                      interpolation_type);
  gimp_image_mask_invalidate (gimage);

  /*  Scale all layers  */
  for (list = GIMP_LIST (gimage->layers)->list; 
       list; 
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      if (! gimp_layer_scale_by_factors (layer, img_scale_w, img_scale_h,
                                         interpolation_type))
	{
	  /* Since 0 < img_scale_w, img_scale_h, failure due to one or more
	   * vanishing scaled layer dimensions. Implicit delete implemented
	   * here. Upstream warning implemented in resize_check_layer_scaling()
	   * [resize.c line 1295], which offers the user the chance to bail out.
	   */
          remove = g_slist_append (remove, layer);
        }

      if (progress_func)
        {
          (* progress_func) (0, num_channels + num_layers,
                             progress_current++,
                             progress_data);
        }
    }

  /* We defer removing layers lost to scaling until now so as not to mix
   * the operations of iterating over and removal from gimage->layers.
   */  
  for (slist = remove; slist; slist = g_slist_next (slist))
    {
      layer = slist->data;
      gimp_image_remove_layer (gimage, layer);
    }
  g_slist_free (remove);

  /*  Scale any Guides  */
  for (list = gimage->guides; list; list = g_list_next (list))
    {
      guide = (GimpGuide *) list->data;

      switch (guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  undo_push_image_guide (gimage, guide);
	  guide->position = (guide->position * new_height) / old_height;
	  break;
	case ORIENTATION_VERTICAL:
	  undo_push_image_guide (gimage, guide);
	  guide->position = (guide->position * new_width) / old_width;
	  break;
	default:
	  g_error("Unknown guide orientation II.\n");
	}
    }

  /*  Make sure the projection matches the gimage size  */
  gimp_image_projection_allocate (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  undo_push_group_end (gimage);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));

  gimp_unset_busy (gimage->gimp);
}

/**
 * gimp_image_check_scaling:
 * @gimage:     A #GimpImage.
 * @new_width:  The new width.
 * @new_height: The new height.
 * 
 * Inventory the layer list in gimage and return #TRUE if, after
 * scaling, they all retain positive x and y pixel dimensions.
 * 
 * Return value: #TRUE if scaling the image will shrink none of it's
 *               layers completely away.
 **/
gboolean 
gimp_image_check_scaling (const GimpImage *gimage,
			  gint             new_width,
			  gint             new_height)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer;

      layer = (GimpLayer *) list->data;

      if (! gimp_layer_check_scaling (layer, new_width, new_height))
	return FALSE;
    }

  return TRUE;
}
