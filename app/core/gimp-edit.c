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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile-manager-crop.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpedit.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimpimage-new.h"
#include "gimplayer.h"
#include "gimplist.h"

#include "app_procs.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimpparasite.h"
#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


TileManager *
gimp_edit_cut (GimpImage    *gimage,
	       GimpDrawable *drawable)
{
  TileManager *cut;
  TileManager *cropped_cut;
  gboolean     empty;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_CUT_UNDO);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  Next, cut the mask portion from the gimage  */
  cut = gimage_mask_extract (gimage, drawable, TRUE, FALSE, TRUE);

  if (cut)
    gimp_image_new_set_have_current_cut_buffer (gimage->gimp);

  /*  Only crop if the gimage mask wasn't empty  */
  if (cut && ! empty)
    {
      cropped_cut = tile_manager_crop (cut, 0);

      if (cropped_cut != cut)
	{
	  tile_manager_destroy (cut);
	  cut = NULL;
	}
    }
  else if (cut)
    cropped_cut = cut;
  else
    cropped_cut = NULL;

  /*  end the group undo  */
  undo_push_group_end (gimage);

  if (cropped_cut)
    {
      /*  Free the old global edit buffer  */
      if (gimage->gimp->global_buffer)
	tile_manager_destroy (gimage->gimp->global_buffer);

      /*  Set the global edit buffer  */
      gimage->gimp->global_buffer = cropped_cut;
    }

  return cropped_cut;
}

TileManager *
gimp_edit_copy (GimpImage    *gimage,
		GimpDrawable *drawable)
{
  TileManager *copy;
  TileManager *cropped_copy;
  gboolean     empty;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  First, copy the masked portion of the gimage  */
  copy = gimage_mask_extract (gimage, drawable, FALSE, FALSE, TRUE);

  if (copy)
    gimp_image_new_set_have_current_cut_buffer (gimage->gimp);

  /*  Only crop if the gimage mask wasn't empty  */
  if (copy && ! empty)
    {
      cropped_copy = tile_manager_crop (copy, 0);

      if (cropped_copy != copy)
	{
	  tile_manager_destroy (copy);
	  copy = NULL;
	}
    }
  else if (copy)
    cropped_copy = copy;
  else
    cropped_copy = NULL;

  if (cropped_copy)
    {
      /*  Free the old global edit buffer  */
      if (gimage->gimp->global_buffer)
	tile_manager_destroy (gimage->gimp->global_buffer);

      /*  Set the global edit buffer  */
      gimage->gimp->global_buffer = cropped_copy;
    }

  return cropped_copy;
}

GimpLayer *
gimp_edit_paste (GimpImage    *gimage,
		 GimpDrawable *drawable,
		 TileManager  *paste,
		 gboolean      paste_into)
{
  GimpLayer *layer;
  gint       x1, y1, x2, y2;
  gint       cx, cy;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable), NULL);

  /*  Make a new layer: if drawable == NULL,
   *  user is pasting into an empty image.
   */

  if (drawable != NULL)
    layer = gimp_layer_new_from_tiles (gimage,
				       gimp_drawable_type_with_alpha (drawable),
				       paste, 
				       _("Pasted Layer"),
				       OPAQUE_OPACITY, NORMAL_MODE);
  else
    layer = gimp_layer_new_from_tiles (gimage,
				       gimp_image_base_type_with_alpha (gimage),
				       paste, 
				       _("Pasted Layer"),
				       OPAQUE_OPACITY, NORMAL_MODE);

  if (! layer)
    return NULL;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

  /*  Set the offsets to the center of the image  */
  if (drawable != NULL)
    {
      gimp_drawable_offsets (drawable, &cx, &cy);
      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      cx += (x1 + x2) >> 1;
      cy += (y1 + y2) >> 1;
    }
  else
    {
      cx = gimage->width  >> 1;
      cy = gimage->height >> 1;
    }

  GIMP_DRAWABLE (layer)->offset_x = cx - (GIMP_DRAWABLE (layer)->width  >> 1);
  GIMP_DRAWABLE (layer)->offset_y = cy - (GIMP_DRAWABLE (layer)->height >> 1);

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimage_mask_is_empty (gimage) && ! paste_into)
    gimp_channel_clear (gimp_image_get_mask (gimage));

  /*  if there's a drawable, add a new floating selection  */
  if (drawable != NULL)
    {
      floating_sel_attach (layer, drawable);
    }
  else
    {
      gimp_drawable_set_gimage (GIMP_DRAWABLE (layer), gimage);
      gimp_image_add_layer (gimage, layer, 0);
    }

  /*  end the group undo  */
  undo_push_group_end (gimage);

  return layer;
}

GimpImage *
gimp_edit_paste_as_new (Gimp        *gimp,
			GimpImage   *invoke,
			TileManager *paste)
{
  GimpImage    *gimage;
  GimpLayer    *layer;
  GimpParasite *comment_parasite;
  GDisplay     *gdisp;

  /*  create a new image  (always of type RGB)  */
  gimage = gimage_new (gimp,
		       tile_manager_width (paste), 
		       tile_manager_height (paste), 
		       RGB);
  gimp_image_undo_disable (gimage);

  if (invoke)
    {
      gimp_image_set_resolution (gimage,
				 invoke->xresolution, invoke->yresolution);
      gimp_image_set_unit (gimage, invoke->unit);
    }

  if (gimprc.default_comment)
    {
      comment_parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (gimprc.default_comment)+1,
                                            (gpointer) gimprc.default_comment);
      gimp_image_parasite_attach (gimage, comment_parasite);
      gimp_parasite_free (comment_parasite);
    }

  layer = gimp_layer_new_from_tiles (gimage,
				     gimp_image_base_type_with_alpha (gimage),
				     paste, 
				     _("Pasted Layer"),
				     OPAQUE_OPACITY, NORMAL_MODE);

  if (layer)
    {
      /*  add the new layer to the image  */
      gimp_drawable_set_gimage (GIMP_DRAWABLE (layer), gimage);
      gimp_image_add_layer (gimage, layer, 0);

      gimp_image_undo_enable (gimage);

      gdisp = gdisplay_new (gimage, 0x0101);
      gimp_context_set_display (gimp_context_get_user (), gdisp);

      return gimage;
    }

  return NULL;
}

gboolean
gimp_edit_clear (GimpImage    *gimage,
		 GimpDrawable *drawable)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         x1, y1, x2, y2;
  guchar       col[MAX_CHANNELS];

  g_return_val_if_fail (gimage != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (drawable != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimp_image_get_background (gimage, drawable, col);
  if (gimp_drawable_has_alpha (drawable))
    col [gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;

  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return TRUE;  /*  nothing to do, but the clear succeded  */

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_region (&bufPR, col);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE, OPAQUE_OPACITY,
			  ERASE_MODE, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable,
		   x1, y1,
		   (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}

gboolean
gimp_edit_fill (GimpImage    *gimage,
		GimpDrawable *drawable,
		GimpFillType  fill_type)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         x1, y1, x2, y2;
  guchar       col[MAX_CHANNELS];

  g_return_val_if_fail (gimage != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (drawable != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_has_alpha (drawable))
    col [gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;

  switch (fill_type)
    {
    case FOREGROUND_FILL:
      gimp_image_get_foreground (gimage, drawable, col);
      break;

    case BACKGROUND_FILL:
      gimp_image_get_background (gimage, drawable, col);
      break;

    case WHITE_FILL:
      col[RED_PIX]   = 255;
      col[GREEN_PIX] = 255;
      col[BLUE_PIX]  = 255;
      break;

    case TRANSPARENT_FILL:
      col[RED_PIX]   = 0;
      col[GREEN_PIX] = 0;
      col[BLUE_PIX]  = 0;
      if (gimp_drawable_has_alpha (drawable))
	col [gimp_drawable_bytes (drawable) - 1] = TRANSPARENT_OPACITY;
      break;

    case NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */

    default:
      g_warning ("%s(): unknown fill type", G_GNUC_FUNCTION);
      gimp_image_get_background (gimage, drawable, col);
      break;
    }

  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return TRUE;  /*  nothing to do, but the fill succeded  */

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_region (&bufPR, col);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE, OPAQUE_OPACITY,
			  NORMAL_MODE, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable,
		   x1, y1,
		   (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}
