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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimp-edit.h"
#include "gimpbuffer.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimpselection.h"

#include "gimp-intl.h"


/*  local function protypes  */

static GimpBuffer * gimp_edit_extract         (GimpImage            *image,
                                               GimpPickable         *pickable,
                                               GimpContext          *context,
                                               gboolean              cut_pixels,
                                               GError              **error);
static gboolean     gimp_edit_fill_internal   (GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GimpContext          *context,
                                               GimpFillType          fill_type,
                                               gdouble               opacity,
                                               GimpLayerModeEffects  paint_mode,
                                               const gchar          *undo_desc);


/*  public functions  */

const GimpBuffer *
gimp_edit_cut (GimpImage     *image,
               GimpDrawable  *drawable,
               GimpContext   *context,
               GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (drawable),
                              context, TRUE, error);

  if (buffer)
    {
      gimp_set_global_buffer (image->gimp, buffer);
      g_object_unref (buffer);

      return image->gimp->global_buffer;
    }

  return NULL;
}

const GimpBuffer *
gimp_edit_copy (GimpImage     *image,
                GimpDrawable  *drawable,
                GimpContext   *context,
                GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (drawable),
                              context, FALSE, error);

  if (buffer)
    {
      gimp_set_global_buffer (image->gimp, buffer);
      g_object_unref (buffer);

      return image->gimp->global_buffer;
    }

  return NULL;
}

const GimpBuffer *
gimp_edit_copy_visible (GimpImage    *image,
                        GimpContext  *context,
                        GError      **error)
{
  GimpProjection *projection;
  GimpBuffer     *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  projection = gimp_image_get_projection (image);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (projection),
                              context, FALSE, error);

  if (buffer)
    {
      gimp_set_global_buffer (image->gimp, buffer);
      g_object_unref (buffer);

      return image->gimp->global_buffer;
    }

  return NULL;
}

GimpLayer *
gimp_edit_paste (GimpImage    *image,
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
  gint           image_width;
  gint           image_height;
  gint           width;
  gint           height;
  gint           offset_x;
  gint           offset_y;
  gboolean       clamp_to_image = TRUE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
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
    type = gimp_image_base_type_with_alpha (image);

  layer = gimp_layer_new_from_tiles (paste->tiles, image, type,
                                     _("Pasted Layer"),
                                     GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  if (! layer)
    return NULL;

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  width  = gimp_item_get_width  (GIMP_ITEM (layer));
  height = gimp_item_get_height (GIMP_ITEM (layer));

  if (viewport_width  == image_width &&
      viewport_height == image_height)
    {
      /* if the whole image is visible, act as if there was no viewport */

      viewport_x      = 0;
      viewport_y      = 0;
      viewport_width  = 0;
      viewport_height = 0;
    }

  if (drawable)
    {
      /*  if pasting to a drawable  */

      gint     off_x, off_y;
      gint     x1, y1, x2, y2;
      gint     paste_x, paste_y;
      gint     paste_width, paste_height;
      gboolean have_mask;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      have_mask = gimp_item_mask_bounds (GIMP_ITEM (drawable),
                                         &x1, &y1, &x2, &y2);

      if (! have_mask         && /* if we have no mask */
          viewport_width  > 0 && /* and we have a viewport */
          viewport_height > 0 &&
          (width  < (x2 - x1) || /* and the paste is smaller than the target */
           height < (y2 - y1)) &&

          /* and the viewport intersects with the target */
          gimp_rectangle_intersect (viewport_x, viewport_y,
                                    viewport_width, viewport_height,
                                    off_x, off_y,
                                    x2 - x1, y2 - y1,
                                    &paste_x, &paste_y,
                                    &paste_width, &paste_height))
        {
          /*  center on the viewport  */

          offset_x = paste_x + (paste_width - width)  / 2;
          offset_y = paste_y + (paste_height- height) / 2;
        }
      else
        {
          /*  otherwise center on the target  */

          offset_x = off_x + ((x1 + x2) - width)  / 2;
          offset_y = off_y + ((y1 + y2) - height) / 2;

          /*  and keep it that way  */
          clamp_to_image = FALSE;
        }
    }
  else if (viewport_width  > 0 &&  /* if we have a viewport */
           viewport_height > 0 &&
           (width  < image_width || /* and the paste is       */
            height < image_height)) /* smaller than the image */
    {
      /*  center on the viewport  */

      offset_x = viewport_x + (viewport_width  - width)  / 2;
      offset_y = viewport_y + (viewport_height - height) / 2;
    }
  else
    {
      /*  otherwise center on the image  */

      offset_x = (image_width  - width)  / 2;
      offset_y = (image_height - height) / 2;

      /*  and keep it that way  */
      clamp_to_image = FALSE;
    }

  if (clamp_to_image)
    {
      /*  Ensure that the pasted layer is always within the image, if it
       *  fits and aligned at top left if it doesn't. (See bug #142944).
       */
      offset_x = MIN (offset_x, image_width  - width);
      offset_y = MIN (offset_y, image_height - height);
      offset_x = MAX (offset_x, 0);
      offset_y = MAX (offset_y, 0);
    }

  gimp_item_set_offset (GIMP_ITEM (layer), offset_x, offset_y);

  /*  Start a group undo  */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               C_("undo-type", "Paste"));

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)) && ! paste_into)
    gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);

  /*  if there's a drawable, add a new floating selection  */
  if (drawable)
    floating_sel_attach (layer, drawable);
  else
    gimp_image_add_layer (image, layer, NULL, 0, TRUE);

  /*  end the group undo  */
  gimp_image_undo_group_end (image);

  return layer;
}

const gchar *
gimp_edit_named_cut (GimpImage     *image,
                     const gchar   *name,
                     GimpDrawable  *drawable,
                     GimpContext   *context,
                     GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (drawable),
                              context, TRUE, error);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy (GimpImage     *image,
                      const gchar   *name,
                      GimpDrawable  *drawable,
                      GimpContext   *context,
                      GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (drawable),
                              context, FALSE, error);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy_visible (GimpImage    *image,
                              const gchar  *name,
                              GimpContext  *context,
                              GError      **error)
{
  GimpProjection *projection;
  GimpBuffer     *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  projection = gimp_image_get_projection (image);

  buffer = gimp_edit_extract (image, GIMP_PICKABLE (projection),
                              context, FALSE, error);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}

gboolean
gimp_edit_clear (GimpImage    *image,
                 GimpDrawable *drawable,
                 GimpContext  *context)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  return gimp_edit_fill_internal (image, drawable, context,
                                  GIMP_TRANSPARENT_FILL,
                                  GIMP_OPACITY_OPAQUE, GIMP_ERASE_MODE,
                                  C_("undo-type", "Clear"));
}

gboolean
gimp_edit_fill (GimpImage    *image,
                GimpDrawable *drawable,
                GimpContext  *context,
                GimpFillType  fill_type)
{
  const gchar *undo_desc;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      undo_desc = C_("undo-type", "Fill with Foreground Color");
      break;

    case GIMP_BACKGROUND_FILL:
      undo_desc = C_("undo-type", "Fill with Background Color");
      break;

    case GIMP_WHITE_FILL:
      undo_desc = C_("undo-type", "Fill with White");
      break;

    case GIMP_TRANSPARENT_FILL:
      undo_desc = C_("undo-type", "Fill with Transparency");
      break;

    case GIMP_PATTERN_FILL:
      undo_desc = C_("undo-type", "Fill with Pattern");
      break;

    case GIMP_NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */

    default:
      g_warning ("%s: unknown fill type", G_STRFUNC);
      return gimp_edit_fill (image, drawable, context, GIMP_BACKGROUND_FILL);
    }

  return gimp_edit_fill_internal (image, drawable, context,
                                  fill_type,
                                  GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE,
                                  undo_desc);
}

gboolean
gimp_edit_fade (GimpImage   *image,
                GimpContext *context)
{
  GimpDrawableUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

  if (undo && undo->src2_tiles)
    {
      GimpDrawable     *drawable;
      TileManager      *src2_tiles;
      PixelRegion       src2PR;

      drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);

      g_object_ref (undo);
      src2_tiles = tile_manager_ref (undo->src2_tiles);

      gimp_image_undo (image);

      pixel_region_init (&src2PR, src2_tiles,
                         0, 0,
                         undo->width, undo->height,
                         FALSE);

      gimp_drawable_apply_region (drawable, &src2PR,
                                  TRUE,
                                  gimp_object_get_name (undo),
                                  gimp_context_get_opacity (context),
                                  gimp_context_get_paint_mode (context),
                                  NULL, NULL,
                                  undo->x,
                                  undo->y);

      tile_manager_unref (src2_tiles);
      g_object_unref (undo);

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static GimpBuffer *
gimp_edit_extract (GimpImage     *image,
                   GimpPickable  *pickable,
                   GimpContext   *context,
                   gboolean       cut_pixels,
                   GError       **error)
{
  TileManager *tiles;
  gint         offset_x;
  gint         offset_y;

  if (cut_pixels)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_CUT, C_("undo-type", "Cut"));

  /*  Cut/copy the mask portion from the image  */
  tiles = gimp_selection_extract (GIMP_SELECTION (gimp_image_get_mask (image)),
                                  pickable, context,
                                  cut_pixels, FALSE, FALSE,
                                  &offset_x, &offset_y, error);

  if (cut_pixels)
    gimp_image_undo_group_end (image);

  if (tiles)
    {
      GimpBuffer *buffer = gimp_buffer_new (tiles, _("Global Buffer"),
                                            offset_x, offset_y, FALSE);

      tile_manager_unref (tiles);

      return buffer;
    }

  return NULL;
}

static gboolean
gimp_edit_fill_internal (GimpImage            *image,
                         GimpDrawable         *drawable,
                         GimpContext          *context,
                         GimpFillType          fill_type,
                         gdouble               opacity,
                         GimpLayerModeEffects  paint_mode,
                         const gchar          *undo_desc)
{
  TileManager   *buf_tiles;
  PixelRegion    bufPR;
  gint           x, y, width, height;
  GimpImageType  drawable_type;
  gint           tiles_bytes;
  guchar         col[MAX_CHANNELS];
  TempBuf       *pat_buf = NULL;
  gboolean       new_buf;

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return TRUE;  /*  nothing to do, but the fill succeded  */

  drawable_type = gimp_drawable_type (drawable);
  tiles_bytes   = gimp_drawable_bytes (drawable);

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      gimp_image_get_foreground (image, context, drawable_type, col);
      break;

    case GIMP_BACKGROUND_FILL:
    case GIMP_TRANSPARENT_FILL:
      gimp_image_get_background (image, context, drawable_type, col);
      break;

    case GIMP_WHITE_FILL:
      {
        guchar tmp_col[MAX_CHANNELS];

        tmp_col[RED]   = 255;
        tmp_col[GREEN] = 255;
        tmp_col[BLUE]  = 255;
        gimp_image_transform_color (image, drawable_type, col,
                                    GIMP_RGB, tmp_col);
      }
      break;

    case GIMP_PATTERN_FILL:
      {
        GimpPattern *pattern = gimp_context_get_pattern (context);

        pat_buf = gimp_image_transform_temp_buf (image, drawable_type,
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
                              NULL, NULL, x, y);

  tile_manager_unref (buf_tiles);

  gimp_drawable_update (drawable, x, y, width, height);

  return TRUE;
}
