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

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimpselection.h"

#include "gimp-intl.h"


/*  public functions  */

TileManager *
gimp_image_mask_extract (GimpImage    *gimage,
                         GimpDrawable *drawable,
                         gboolean      cut_image,
                         gboolean      keep_indexed,
                         gboolean      add_alpha)
{
  TileManager       *tiles;
  GimpChannel       *sel_mask;
  PixelRegion        srcPR, destPR, maskPR;
  guchar             bg_color[MAX_CHANNELS];
  GimpImageBaseType  base_type = GIMP_RGB;
  gint               bytes     = 0;
  gint               x1, y1, x2, y2;
  gint               off_x, off_y;
  gboolean           non_empty;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  non_empty = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  if (non_empty && (!(x2 - x1) || !(y2 - y1)))
    {
      g_message (_("Unable to cut or copy because the\n"
		   "selected region is empty."));
      return NULL;
    }

  /*  How many bytes in the temp buffer?  */
  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)))
    {
    case GIMP_RGB:
      bytes = add_alpha ? 4 : drawable->bytes;
      base_type = GIMP_RGB;
      break;

    case GIMP_GRAY:
      bytes = add_alpha ? 2 : drawable->bytes;
      base_type = GIMP_GRAY;
      break;

    case GIMP_INDEXED:
      if (keep_indexed)
	{
	  bytes = add_alpha ? 2 : drawable->bytes;
	  base_type = GIMP_GRAY;
	}
      else
	{
	  bytes = add_alpha ? 4 : drawable->bytes;
	  base_type = GIMP_INDEXED;
	}
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /*  get the selection mask */
  if (non_empty)
    sel_mask = gimp_image_get_mask (gimage);
  else
    sel_mask = NULL;

  gimp_image_get_background (gimage, drawable, bg_color);

  /*  If a cut was specified, and the selection mask is not empty,
   *  push an undo
   */
  if (cut_image && non_empty)
    gimp_drawable_push_undo (drawable, NULL, x1, y1, x2, y2, NULL, FALSE);

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  /*  Allocate the temp buffer  */
  tiles = tile_manager_new (x2 - x1, y2 - y1, bytes);
  tile_manager_set_offsets (tiles, x1 + off_x, y1 + off_y);

  /* configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     x1, y1,
                     x2 - x1, y2 - y1,
                     cut_image);
  pixel_region_init (&destPR, tiles,
                     0, 0,
                     x2 - x1, y2 - y1,
                     TRUE);

  if (non_empty) /*  If there is a selection, extract from it  */
    {
      pixel_region_init (&maskPR, GIMP_DRAWABLE (sel_mask)->tiles,
			 (x1 + off_x), (y1 + off_y), (x2 - x1), (y2 - y1),
			 FALSE);

      extract_from_region (&srcPR, &destPR, &maskPR,
			   gimp_drawable_cmap (drawable),
			   bg_color, base_type,
			   gimp_drawable_has_alpha (drawable), cut_image);

      if (cut_image)
	{
	  /*  Clear the region  */
	  gimp_channel_clear (sel_mask, NULL, TRUE);

	  /*  Update the region  */
	  gimp_image_update (gimage, 
			     x1 + off_x, y1 + off_y,
			     (x2 - x1), (y2 - y1));

	  /*  Invalidate the preview  */
	  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
	}
    }
  else /*  Otherwise, get the entire active layer  */
    {
      /*  If the layer is indexed...we need to extract pixels  */
      if (base_type == GIMP_INDEXED && !keep_indexed)
	extract_from_region (&srcPR, &destPR, NULL,
			     gimp_drawable_cmap (drawable),
			     bg_color, base_type,
			     gimp_drawable_has_alpha (drawable), FALSE);
      /*  If the layer doesn't have an alpha channel, add one  */
      else if (bytes > srcPR.bytes)
	add_alpha_region (&srcPR, &destPR);
      /*  Otherwise, do a straight copy  */
      else
	copy_region (&srcPR, &destPR);

      /*  If we're cutting, remove either the layer (or floating selection),
       *  the layer mask, or the channel
       */
      if (cut_image)
	{
	  if (GIMP_IS_LAYER (drawable))
	    {
	      if (gimp_layer_is_floating_sel (GIMP_LAYER (drawable)))
		floating_sel_remove (GIMP_LAYER (drawable));
	      else
		gimp_image_remove_layer (gimage, GIMP_LAYER (drawable));
	    }
	  else if (GIMP_IS_LAYER_MASK (drawable))
	    {
	      gimp_layer_apply_mask (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)),
				     GIMP_MASK_DISCARD, TRUE);
	    }
	  else if (GIMP_IS_CHANNEL (drawable))
	    {
	      gimp_image_remove_channel (gimage, GIMP_CHANNEL (drawable));
	    }
	}
    }

  return tiles;
}

GimpLayer *
gimp_image_mask_float (GimpImage    *gimage,
                       GimpDrawable *drawable,
                       gboolean      cut_image,
                       gint          off_x,    /* optional offset */
                       gint          off_y)
{
  GimpLayer   *layer;
  GimpChannel *mask = gimp_image_get_mask (gimage);
  TileManager *tiles;
  gboolean     non_empty;
  gint         x1, y1;
  gint         x2, y2;

  /*  Make sure there is a region to float...  */
  non_empty = gimp_drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
  if (! non_empty || (x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_message (_("Cannot float selection because the\n"
		   "selected region is empty."));
      return NULL;
    }

  /*  Start an undo group  */
  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_FS_FLOAT,
                               _("Float Selection"));

  /*  Cut or copy the selected region  */
  tiles = gimp_image_mask_extract (gimage, drawable, cut_image, FALSE, TRUE);

  /*  Clear the selection as if we had cut the pixels  */
  if (! cut_image)
    gimp_channel_clear (mask, NULL, TRUE);

  /* Create a new layer from the buffer, using the drawable's type
   *  because it may be different from the image's type if we cut from
   *  a channel or layer mask
   */
  layer = gimp_layer_new_from_tiles (tiles,
                                     gimage,
                                     gimp_drawable_type_with_alpha (drawable),
				     _("Floating Selection"),
				     GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  /*  Set the offsets  */
  tile_manager_get_offsets (tiles, &x1, &y1);
  GIMP_ITEM (layer)->offset_x = x1 + off_x;
  GIMP_ITEM (layer)->offset_y = y1 + off_y;

  /*  Free the temp buffer  */
  tile_manager_unref (tiles);

  /*  Add the floating layer to the gimage  */
  floating_sel_attach (layer, drawable);

  /*  End an undo group  */
  gimp_image_undo_group_end (gimage);

  /*  invalidate the gimage's boundary variables  */
  mask->boundary_known = FALSE;

  return layer;
}

void
gimp_image_mask_load (GimpImage   *gimage,
                      GimpChannel *channel)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  gimp_channel_push_undo (gimp_image_get_mask (gimage),
                          _("Selection from Channel"));

  /*  load the specified channel to the gimage mask  */
  gimp_channel_load (gimp_image_get_mask (gimage), channel, FALSE);

  gimp_image_mask_changed (gimage);
}

GimpChannel *
gimp_image_mask_save (GimpImage *gimage)
{
  GimpChannel *mask;
  GimpChannel *new_channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  mask = gimp_image_get_mask (gimage);

  new_channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (mask),
                                                   GIMP_TYPE_CHANNEL,
                                                   FALSE));

  /*  saved selections are not visible by default  */
  gimp_drawable_set_visible (GIMP_DRAWABLE (new_channel), FALSE, FALSE);

  gimp_image_add_channel (gimage, new_channel, -1);

  return new_channel;
}
