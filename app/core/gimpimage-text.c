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
#include <pango/pangoft2.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"
#include "text/gimptext.h"

#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimpimage-text.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


GimpLayer *
gimp_image_text_render (GimpImage *gimage,
                        GimpText  *text)
{
  PangoFontDescription *font_desc;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoRectangle        ink;
  PangoRectangle        logical;
  GimpLayer            *layer = NULL;
  gdouble               factor;
  gdouble               xres;
  gdouble               yres;
  gint                  size;
  gint                  border;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  if (!text->str)
    return NULL;

  gimp_image_get_resolution (gimage, &xres, &yres);

  switch (text->unit)
    {
    case GIMP_UNIT_PIXEL:
      size   = PANGO_SCALE * text->size;
      border = text->border;
      break;

    default:
      factor = gimp_unit_get_factor (text->unit);
      g_return_val_if_fail (factor > 0.0, NULL);

      size   = (gdouble) PANGO_SCALE * text->size * yres / factor;
      border = text->border * yres / factor;
      break;
    }

  font_desc = pango_font_description_from_string (text->font);
  g_return_val_if_fail (font_desc != NULL, NULL);
  if (!font_desc)
    return NULL;

  if (size > 1)
    pango_font_description_set_size (font_desc, size);

  context = pango_ft2_get_context (xres, yres);
  layout = pango_layout_new (context);

  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text->str, -1);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  g_print ("ink rect: %d x %d @ %d, %d\n", 
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n", 
           logical.width, logical.height, logical.x, logical.y);

  if (ink.width > 8192 || ink.height > 8192)
    {
      g_message ("Uh, oh, insane text size.");
      return NULL;
    }

  if (ink.width > 0 && ink.height > 0)
    {
      TileManager *mask;
      PixelRegion  textPR;
      PixelRegion  maskPR;
      FT_Bitmap    bitmap;
      guchar      *black = NULL;
      guchar       color[MAX_CHANNELS];
      gint         width;
      gint         height;
      gint         y;

      bitmap.width  = ink.width;
      bitmap.rows   = ink.height;
      bitmap.pitch  = ink.width;
      if (bitmap.pitch & 3)
        bitmap.pitch += 4 - (bitmap.pitch & 3);

      bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);
      
      pango_ft2_render_layout (&bitmap, layout, - ink.x, - ink.y);
     
      width  = ink.width  + 2 * border;
      height = ink.height + 2 * border;

      mask = tile_manager_new (width, height, 1);
      pixel_region_init (&maskPR, mask, 0, 0, width, height, TRUE);

      if (border)
        black = g_malloc0 (width);

      for (y = 0; y < border; y++)
        pixel_region_set_row (&maskPR, 0, y, width, black);
      for (; y < height - border; y++)
        {
          if (border)
            {
              pixel_region_set_row (&maskPR, 0, y, border, black);
              pixel_region_set_row (&maskPR, width - border, y, border, black);
            }
          pixel_region_set_row (&maskPR, border, y, bitmap.width,
                                bitmap.buffer + (y - border) * bitmap.pitch);
        }
      for (; y < height; y++)
        pixel_region_set_row (&maskPR, 0, y, width, black);

      g_free (black);
      g_free (bitmap.buffer);

      layer = gimp_layer_new (gimage,
                              width, height,
                              gimp_image_base_type_with_alpha (gimage),
                              _("Text Layer"),
                              GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      /*  color the layer buffer  */
      gimp_image_get_foreground (gimage, GIMP_DRAWABLE (layer), color);
      color[GIMP_DRAWABLE (layer)->bytes - 1] = OPAQUE_OPACITY;
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
			 0, 0, width, height, TRUE);
      color_region (&textPR, color);

      /*  apply the text mask  */
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
                         0, 0, width, height, TRUE);
      pixel_region_init (&maskPR, mask,
			 0, 0, width, height, FALSE);
      apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);

      tile_manager_destroy (mask);
    }

  g_object_unref (layout);
  g_object_unref (context);  

  return layer;
}

GimpLayer *
text_render (GimpImage    *gimage,
	     GimpDrawable *drawable,
	     gint          text_x,
	     gint          text_y,
	     const gchar  *fontname,
	     const gchar  *text,
	     gint          border,
	     gint          antialias)
{
  GimpText  *gtext;
  GimpLayer *layer;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  if (border < 0)
    border = 0;

  gtext = GIMP_TEXT (g_object_new (GIMP_TYPE_TEXT,
                                   "text",   text,
                                   "font",   fontname,
                                   "border", border,
                                   "unit",   GIMP_UNIT_PIXEL,
                                   NULL));
  gtext->size = -1;

  layer = gimp_image_text_render (gimage, gtext);

  g_object_unref (gtext);

  if (!layer)
    return NULL;

  /*  Start a group undo  */
  undo_push_group_start (gimage, TEXT_UNDO_GROUP);

  /*  Set the layer offsets  */
  GIMP_DRAWABLE (layer)->offset_x = text_x;
  GIMP_DRAWABLE (layer)->offset_y = text_y;

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimp_image_mask_is_empty (gimage))
    gimp_image_mask_clear (gimage);
  
  /*  If the drawable is NULL, create a new layer  */
  if (drawable == NULL)
    gimp_image_add_layer (gimage, layer, -1);
  /*  Otherwise, instantiate the text as the new floating selection */
  else
    floating_sel_attach (layer, drawable);

  /*  end the group undo  */
  undo_push_group_end (gimage);

  return layer;
}

gboolean
text_get_extents (const gchar *fontname,
		  const gchar *text,
		  gint        *width,
		  gint        *height,
		  gint        *ascent,
		  gint        *descent)
{
  PangoFontDescription *font_desc;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoRectangle        rect;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  font_desc = pango_font_description_from_string (fontname);
  if (!font_desc)
    return FALSE;

  /* FIXME: resolution */
  context = pango_ft2_get_context (72.0, 72.0);

  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, &rect, NULL);

  if (width)
    *width = rect.width;
  if (height)
    *height = rect.height;
  if (ascent)
    *ascent = -rect.y;
  if (descent)
    *descent = rect.height + rect.y;

  g_object_unref (layout);
  g_object_unref (context);  

  return TRUE;
}
