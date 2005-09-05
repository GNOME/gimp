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

#include <glib-object.h>

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile-manager-crop.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimp-edit.h"
#include "gimp-utils.h"
#include "gimpbuffer.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"
#include "gimppattern.h"
#include "gimpprojection.h"
#include "gimpselection.h"

#include "gimp-intl.h"


/*  local function protypes  */

static GimpBuffer * gimp_edit_extract         (GimpImage            *gimage,
                                               GimpDrawable         *drawable,
                                               GimpContext          *context,
                                               gboolean              cut_pixels);
static GimpBuffer * gimp_edit_extract_visible (GimpImage            *gimage,
                                               GimpContext          *context);
static GimpBuffer * gimp_edit_make_buffer     (Gimp                 *gimp,
                                               TileManager          *tiles,
                                               gboolean              mask_empty);
static gboolean     gimp_edit_fill_internal   (GimpImage            *gimage,
                                               GimpDrawable         *drawable,
                                               GimpContext          *context,
                                               GimpFillType          fill_type,
                                               gdouble               opacity,
                                               GimpLayerModeEffects  paint_mode,
                                               const gchar          *undo_desc);


/*  public functions  */

const GimpBuffer *
gimp_edit_cut (GimpImage    *gimage,
	       GimpDrawable *drawable,
               GimpContext  *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract (gimage, drawable, context, TRUE);

  if (buffer)
    {
      gimp_set_global_buffer (gimage->gimp, buffer);
      g_object_unref (buffer);

      return gimage->gimp->global_buffer;
    }

  return NULL;
}

const GimpBuffer *
gimp_edit_copy (GimpImage    *gimage,
		GimpDrawable *drawable,
                GimpContext  *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract (gimage, drawable, context, FALSE);

  if (buffer)
    {
      gimp_set_global_buffer (gimage->gimp, buffer);
      g_object_unref (buffer);

      return gimage->gimp->global_buffer;
    }

  return NULL;
}

const GimpBuffer *
gimp_edit_copy_visible (GimpImage   *gimage,
                        GimpContext *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract_visible (gimage, context);

  if (buffer)
    {
      gimp_set_global_buffer (gimage->gimp, buffer);
      g_object_unref (buffer);

      return gimage->gimp->global_buffer;
    }

  return NULL;
}

GimpLayer *
gimp_edit_paste (GimpImage    *gimage,
		 GimpDrawable *drawable,
		 GimpBuffer   *paste,
		 gboolean      paste_into,
                 gint          viewport_x,
                 gint          viewport_y,
                 gint          viewport_width,
                 gint          viewport_height)
{
  GimpLayer     *layer;
  GimpImageType  type;
  gint           center_x;
  gint           center_y;
  gint           offset_x;
  gint           offset_y;
  gint           width;
  gint           height;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (drawable == NULL ||
                        gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_BUFFER (paste), NULL);

  /*  Make a new layer: if drawable == NULL,
   *  user is pasting into an empty image.
   */

  if (drawable)
    type = gimp_drawable_type_with_alpha (drawable);
  else
    type = gimp_image_base_type_with_alpha (gimage);

  layer = gimp_layer_new_from_tiles (paste->tiles, gimage, type,
                                     _("Pasted Layer"),
                                     GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  if (! layer)
    return NULL;

  if (drawable)
    {
      /*  if pasting to a drawable  */

      gint     off_x, off_y;
      gint     x1, y1, x2, y2;
      gint     paste_x, paste_y;
      gint     paste_width, paste_height;
      gboolean have_mask;

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
      have_mask = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      if (! have_mask         &&
          viewport_width  > 0 &&
          viewport_height > 0 &&
          gimp_rectangle_intersect (viewport_x, viewport_y,
                                    viewport_width, viewport_height,
                                    off_x, off_y,
                                    x2 - x1, y2 - y1,
                                    &paste_x, &paste_y,
                                    &paste_width, &paste_height))
        {
          center_x = paste_x + paste_width  / 2;
          center_y = paste_y + paste_height / 2;
        }
      else
        {
          center_x = off_x + (x1 + x2) / 2;
          center_y = off_y + (y1 + y2) / 2;
        }
    }
  else if (viewport_width > 0 && viewport_height > 0)
    {
      /*  if we got a viewport set the offsets to the center of the viewport  */

      center_x = viewport_x + viewport_width  / 2;
      center_y = viewport_y + viewport_height / 2;
    }
  else
    {
      /*  otherwise the offsets to the center of the image  */

      center_x = gimage->width  / 2;
      center_y = gimage->height / 2;
    }

  width  = gimp_item_width  (GIMP_ITEM (layer));
  height = gimp_item_height (GIMP_ITEM (layer));

  offset_x = center_x - width  / 2;
  offset_y = center_y - height / 2;

  /*  Ensure that the pasted layer is always within the image, if it
   *  fits and aligned at top left if it doesn't. (See bug #142944).
   */
  offset_x = MIN (offset_x, gimage->width  - width);
  offset_y = MIN (offset_y, gimage->height - height);
  offset_x = MAX (offset_x, 0);
  offset_y = MAX (offset_y, 0);

  GIMP_ITEM (layer)->offset_x = offset_x;
  GIMP_ITEM (layer)->offset_y = offset_y;

  /*  Start a group undo  */
  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("Paste"));

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (gimage)) && ! paste_into)
    gimp_channel_clear (gimp_image_get_mask (gimage), NULL, TRUE);

  /*  if there's a drawable, add a new floating selection  */
  if (drawable)
    floating_sel_attach (layer, drawable);
  else
    gimp_image_add_layer (gimage, layer, 0);

  /*  end the group undo  */
  gimp_image_undo_group_end (gimage);

  return layer;
}

GimpImage *
gimp_edit_paste_as_new (Gimp       *gimp,
			GimpImage  *invoke,
			GimpBuffer *paste)
{
  GimpImage     *gimage;
  GimpLayer     *layer;
  GimpImageType  type;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (invoke == NULL || GIMP_IS_IMAGE (invoke), NULL);
  g_return_val_if_fail (GIMP_IS_BUFFER (paste), NULL);

  switch (tile_manager_bpp (paste->tiles))
    {
    case 1: type = GIMP_GRAY_IMAGE;  break;
    case 2: type = GIMP_GRAYA_IMAGE; break;
    case 3: type = GIMP_RGB_IMAGE;   break;
    case 4: type = GIMP_RGBA_IMAGE;  break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  /*  create a new image  (always of type GIMP_RGB)  */
  gimage = gimp_create_image (gimp,
			      gimp_buffer_get_width (paste),
                              gimp_buffer_get_height (paste),
			      GIMP_IMAGE_TYPE_BASE_TYPE (type),
			      TRUE);
  gimp_image_undo_disable (gimage);

  if (invoke)
    {
      gimp_image_set_resolution (gimage,
				 invoke->xresolution, invoke->yresolution);
      gimp_image_set_unit (gimage,
                           gimp_image_get_unit (invoke));
    }

  layer = gimp_layer_new_from_tiles (paste->tiles, gimage, type,
				     _("Pasted Layer"),
				     GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  if (! layer)
    {
      g_object_unref (gimage);
      return NULL;
    }

  gimp_image_add_layer (gimage, layer, 0);

  gimp_image_undo_enable (gimage);

  return gimage;
}

const gchar *
gimp_edit_named_cut (GimpImage    *gimage,
                     const gchar  *name,
                     GimpDrawable *drawable,
                     GimpContext  *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract (gimage, drawable, context, TRUE);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (gimage->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (GIMP_OBJECT (buffer));
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy (GimpImage    *gimage,
                      const gchar  *name,
                      GimpDrawable *drawable,
                      GimpContext  *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract (gimage, drawable, context, FALSE);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (gimage->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (GIMP_OBJECT (buffer));
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy_visible (GimpImage   *gimage,
                              const gchar *name,
                              GimpContext *context)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  buffer = gimp_edit_extract_visible (gimage, context);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (gimage->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (GIMP_OBJECT (buffer));
    }

  return NULL;
}

gboolean
gimp_edit_clear (GimpImage    *gimage,
		 GimpDrawable *drawable,
                 GimpContext  *context)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  return gimp_edit_fill_internal (gimage, drawable, context,
                                  GIMP_TRANSPARENT_FILL,
                                  GIMP_OPACITY_OPAQUE, GIMP_ERASE_MODE,
                                  _("Clear"));
}

gboolean
gimp_edit_fill (GimpImage    *gimage,
		GimpDrawable *drawable,
                GimpContext  *context,
		GimpFillType  fill_type)
{
  const gchar *undo_desc;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      undo_desc = _("Fill with FG Color");
      break;

    case GIMP_BACKGROUND_FILL:
      undo_desc = _("Fill with BG Color");
      break;

    case GIMP_WHITE_FILL:
      undo_desc = _("Fill with White");
      break;

    case GIMP_TRANSPARENT_FILL:
      undo_desc = _("Fill with Transparency");
      break;

    case GIMP_PATTERN_FILL:
      undo_desc = _("Fill with Pattern");
      break;

    case GIMP_NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */

    default:
      g_warning ("%s: unknown fill type", G_STRFUNC);
      fill_type = GIMP_BACKGROUND_FILL;
      undo_desc = _("Fill with BG Color");
      break;
    }

  return gimp_edit_fill_internal (gimage, drawable, context,
                                  fill_type,
                                  GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE,
                                  undo_desc);
}


/*  private functions  */

static GimpBuffer *
gimp_edit_extract (GimpImage    *gimage,
                   GimpDrawable *drawable,
                   GimpContext  *context,
                   gboolean      cut_pixels)
{
  TileManager *tiles;
  gboolean     empty;

  /*  See if the gimage mask is empty  */
  empty = gimp_channel_is_empty (gimp_image_get_mask (gimage));

  if (cut_pixels)
    gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_EDIT_CUT, _("Cut"));

  /*  Cut/copy the mask portion from the gimage  */
  tiles = gimp_selection_extract (gimp_image_get_mask (gimage),
                                  drawable, context, cut_pixels, FALSE, FALSE);

  if (cut_pixels)
    gimp_image_undo_group_end (gimage);

  return gimp_edit_make_buffer (gimage->gimp, tiles, empty);
}

static GimpBuffer *
gimp_edit_extract_visible (GimpImage   *gimage,
                           GimpContext *context)
{
  PixelRegion  srcPR, destPR;
  TileManager *tiles;
  gboolean     non_empty;
  gint         x1, y1, x2, y2;

  non_empty = gimp_channel_bounds (gimp_image_get_mask (gimage),
                                   &x1, &y1, &x2, &y2);
  if ((x1 == x2) || (y1 == y2))
    {
      g_message (_("Unable to cut or copy because the "
		   "selected region is empty."));
      return NULL;
    }

  tiles = tile_manager_new (x2 - x1, y2 - y1,
                            gimp_projection_get_bytes (gimage->projection));
  tile_manager_set_offsets (tiles, x1, y1);

  pixel_region_init (&srcPR, gimp_projection_get_tiles (gimage->projection),
		     x1, y1,
                     x2 - x1, y2 - y1,
                     FALSE);
  pixel_region_init (&destPR, tiles,
                     0, 0,
                     x2 - x1, y2 - y1,
                     TRUE);

  /*  use EEKy no-COW copying because sharing tiles with the projection
   *  is buggy as hell, probably because tile_invalidate() doesn't
   *  do what it should  --mitch
   */
  copy_region_nocow (&srcPR, &destPR);

  return gimp_edit_make_buffer (gimage->gimp, tiles, ! non_empty);
}

static GimpBuffer *
gimp_edit_make_buffer (Gimp        *gimp,
                       TileManager *tiles,
                       gboolean     mask_empty)
{
  /*  Only crop if the gimage mask wasn't empty  */
  if (tiles && ! mask_empty)
    {
      TileManager *crop = tile_manager_crop (tiles, 0);

      if (crop != tiles)
        {
          tile_manager_unref (tiles);
          tiles = crop;
        }
    }

  if (tiles)
    return gimp_buffer_new (tiles, _("Global Buffer"), FALSE);

  return NULL;
}

static gboolean
gimp_edit_fill_internal (GimpImage            *gimage,
                         GimpDrawable         *drawable,
                         GimpContext          *context,
                         GimpFillType          fill_type,
                         gdouble               opacity,
                         GimpLayerModeEffects  paint_mode,
                         const gchar          *undo_desc)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         x, y, width, height;
  gint         tiles_bytes;
  guchar       col[MAX_CHANNELS];
  TempBuf     *pat_buf = NULL;
  gboolean     new_buf;

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return TRUE;  /*  nothing to do, but the fill succeded  */

  tiles_bytes = gimp_drawable_bytes (drawable);

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      gimp_image_get_foreground (gimage, drawable, context, col);
      break;

    case GIMP_BACKGROUND_FILL:
    case GIMP_TRANSPARENT_FILL:
      gimp_image_get_background (gimage, drawable, context, col);
      break;

    case GIMP_WHITE_FILL:
      {
        guchar tmp_col[MAX_CHANNELS];

        tmp_col[RED_PIX]   = 255;
        tmp_col[GREEN_PIX] = 255;
        tmp_col[BLUE_PIX]  = 255;
        gimp_image_transform_color (gimage, drawable, col, GIMP_RGB, tmp_col);
      }
      break;

    case GIMP_PATTERN_FILL:
      {
        GimpPattern *pattern = gimp_context_get_pattern (context);

        pat_buf = gimp_image_transform_temp_buf (gimage, drawable,
                                                 pattern->mask, &new_buf);

        if (! gimp_drawable_has_alpha (drawable) &&
            (pat_buf->bytes == 2 || pat_buf->bytes == 4))
          tiles_bytes++;
      }
      break;

    case GIMP_NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */
    }

  buf_tiles = tile_manager_new (width, height, tiles_bytes);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, width, height, TRUE);

  if (pat_buf)
    {
      pattern_region (&bufPR, NULL, pat_buf, 0, 0);

      if (new_buf)
        temp_buf_free (pat_buf);
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable))
        col[gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;

      color_region (&bufPR, col);
    }

  pixel_region_init (&bufPR, buf_tiles, 0, 0, width, height, FALSE);
  gimp_drawable_apply_region (drawable, &bufPR,
                              TRUE, undo_desc,
                              opacity, paint_mode,
                              NULL, x, y);

  tile_manager_unref (buf_tiles);

  gimp_drawable_update (drawable, x, y, width, height);

  return TRUE;
}
