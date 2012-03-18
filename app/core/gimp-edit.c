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

#include <cairo.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

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

static GimpBuffer * gimp_edit_extract (GimpImage     *image,
                                       GimpPickable  *pickable,
                                       GimpContext   *context,
                                       gboolean       cut_pixels,
                                       GError       **error);


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
  gint           center_x;
  gint           center_y;
  gint           offset_x;
  gint           offset_y;
  gint           width;
  gint           height;

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

  layer = gimp_layer_new_from_buffer (gimp_buffer_get_buffer (paste),
                                      image, type,
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

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      have_mask = gimp_item_mask_bounds (GIMP_ITEM (drawable),
                                         &x1, &y1, &x2, &y2);

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

      center_x = gimp_image_get_width  (image) / 2;
      center_y = gimp_image_get_height (image) / 2;
    }

  width  = gimp_item_get_width  (GIMP_ITEM (layer));
  height = gimp_item_get_height (GIMP_ITEM (layer));

  offset_x = center_x - width  / 2;
  offset_y = center_y - height / 2;

  /*  Ensure that the pasted layer is always within the image, if it
   *  fits and aligned at top left if it doesn't. (See bug #142944).
   */
  offset_x = MIN (offset_x, gimp_image_get_width  (image) - width);
  offset_y = MIN (offset_y, gimp_image_get_height (image) - height);
  offset_x = MAX (offset_x, 0);
  offset_y = MAX (offset_y, 0);

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
  GimpRGB background;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  gimp_context_get_background (context, &background);

  return gimp_edit_fill_full (image, drawable,
                              &background, NULL,
                              GIMP_OPACITY_OPAQUE, GIMP_ERASE_MODE,
                              C_("undo-type", "Clear"));
}

gboolean
gimp_edit_fill (GimpImage            *image,
                GimpDrawable         *drawable,
                GimpContext          *context,
                GimpFillType          fill_type,
                gdouble               opacity,
                GimpLayerModeEffects  paint_mode)
{
  GimpRGB      color;
  GimpPattern *pattern = NULL;
  const gchar *undo_desc;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      gimp_context_get_foreground (context, &color);
      undo_desc = C_("undo-type", "Fill with Foreground Color");
      break;

    case GIMP_BACKGROUND_FILL:
      gimp_context_get_background (context, &color);
      undo_desc = C_("undo-type", "Fill with Background Color");
      break;

    case GIMP_WHITE_FILL:
      gimp_rgba_set (&color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
      undo_desc = C_("undo-type", "Fill with White");
      break;

    case GIMP_TRANSPARENT_FILL:
      gimp_context_get_background (context, &color);
      undo_desc = C_("undo-type", "Fill with Transparency");
      break;

    case GIMP_PATTERN_FILL:
      pattern = gimp_context_get_pattern (context);
      undo_desc = C_("undo-type", "Fill with Pattern");
      break;

    case GIMP_NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */

    default:
      g_warning ("%s: unknown fill type", G_STRFUNC);
      return gimp_edit_fill (image, drawable,
                             context, GIMP_BACKGROUND_FILL,
                             GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
    }

  return gimp_edit_fill_full (image, drawable,
                              &color, pattern,
                              opacity, paint_mode,
                              undo_desc);
}

gboolean
gimp_edit_fill_full (GimpImage            *image,
                     GimpDrawable         *drawable,
                     const GimpRGB        *color,
                     GimpPattern          *pattern,
                     gdouble               opacity,
                     GimpLayerModeEffects  paint_mode,
                     const gchar          *undo_desc)
{
  TileManager *buf_tiles;
  GeglBuffer  *dest_buffer;
  PixelRegion  bufPR;
  gint         x, y, width, height;
  gint         tiles_bytes;
  const Babl  *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (color != NULL || pattern != NULL, FALSE);


  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return TRUE;  /*  nothing to do, but the fill succeded  */

  tiles_bytes = gimp_drawable_bytes (drawable);
  format      = gimp_drawable_get_format (drawable);

  if (pattern)
    {
      if (! gimp_drawable_has_alpha (drawable) &&
          (pattern->mask->bytes == 2 || pattern->mask->bytes == 4))
        {
          tiles_bytes++;
          format = gimp_drawable_get_format_with_alpha (drawable);
        }
    }

  buf_tiles = tile_manager_new (width, height, tiles_bytes);

  dest_buffer = gimp_tile_manager_create_buffer (buf_tiles, format, TRUE);

  if (pattern)
    {
      GeglBuffer *src_buffer = gimp_pattern_create_buffer (pattern);

      gegl_buffer_set_pattern (dest_buffer, NULL, src_buffer, 0, 0);

      g_object_unref (src_buffer);
    }
  else
    {
      GeglColor *gegl_color = gegl_color_new (NULL);

      gimp_gegl_color_set_rgba (gegl_color, color);
      gegl_buffer_set_color (dest_buffer, NULL, gegl_color);

      g_object_unref (gegl_color);
    }

  g_object_unref (dest_buffer);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, width, height, FALSE);
  gimp_drawable_apply_region (drawable, &bufPR,
                              TRUE, undo_desc,
                              opacity, paint_mode,
                              NULL, NULL, x, y);

  tile_manager_unref (buf_tiles);

  gimp_drawable_update (drawable, x, y, width, height);

  return TRUE;
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
      GeglBuffer *temp;
      GimpBuffer *buffer;

      temp = gimp_tile_manager_create_buffer (tiles, NULL, TRUE);
      tile_manager_unref (tiles);

      buffer = gimp_buffer_new (temp, _("Global Buffer"),
                                GIMP_IMAGE_TYPE_FROM_BYTES (tile_manager_bpp (tiles)),
                                offset_x, offset_y, FALSE);
      g_object_unref (temp);

      return buffer;
    }

  return NULL;
}
