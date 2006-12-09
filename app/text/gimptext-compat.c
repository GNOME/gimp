/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"

#include "text-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer-floating-sel.h"

#include "gimpfont-utils.h"

#include "gimptext.h"
#include "gimptext-compat.h"
#include "gimptextlayer.h"

#include "gimp-intl.h"


GimpLayer *
text_render (GimpImage    *image,
             GimpDrawable *drawable,
             GimpContext  *context,
             gint          text_x,
             gint          text_y,
             const gchar  *fontname,
             const gchar  *text,
             gint          border,
             gboolean      antialias)
{
  PangoFontDescription *desc;
  GimpText             *gtext;
  GimpLayer            *layer;
  GimpRGB               color;
  gchar                *font;
  gdouble               size;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (drawable == NULL ||
                        gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (fontname != NULL, NULL);
  g_return_val_if_fail (text != NULL, NULL);

  if (border < 0)
    border = 0;

  desc = pango_font_description_from_string (fontname);
  size = PANGO_PIXELS (pango_font_description_get_size (desc));

  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);
  font = gimp_font_util_pango_font_description_to_string (desc);

  pango_font_description_free (desc);

  gimp_context_get_foreground (context, &color);

  gtext = g_object_new (GIMP_TYPE_TEXT,
                        "text",      text,
                        "font",      font,
                        "font-size", size,
                        "antialias", antialias,
                        "border",    border,
                        "color",     &color,
                        NULL);

  g_free (font);

  layer = gimp_text_layer_new (image, gtext);

  g_object_unref (gtext);

  if (!layer)
    return NULL;

  /*  Start a group undo  */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  /*  Set the layer offsets  */
  GIMP_ITEM (layer)->offset_x = text_x;
  GIMP_ITEM (layer)->offset_y = text_y;

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);

  /*  If the drawable is NULL, create a new layer  */
  if (drawable == NULL)
    gimp_image_add_layer (image, layer, -1);
  /*  Otherwise, instantiate the text as the new floating selection */
  else
    floating_sel_attach (layer, drawable);

  /*  end the group undo  */
  gimp_image_undo_group_end (image);

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
  PangoFontMap         *fontmap;
  PangoRectangle        rect;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  /* FIXME: resolution */
  fontmap = pango_ft2_font_map_new ();
  pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), 72.0, 72.0);
  context = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (fontmap));
  g_object_unref (fontmap);

  layout = pango_layout_new (context);
  g_object_unref (context);

  font_desc = pango_font_description_from_string (fontname);
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, NULL, &rect);

  if (width)
    *width = rect.width;
  if (height)
    *height = rect.height;

  if (ascent || descent)
    {
      PangoLayoutIter *iter;
      PangoLayoutLine *line;

      iter = pango_layout_get_iter (layout);
      line = pango_layout_iter_get_line (iter);
      pango_layout_iter_free (iter);

      pango_layout_line_get_pixel_extents (line, NULL, &rect);

      if (ascent)
        *ascent = PANGO_ASCENT (rect);
      if (descent)
        *descent = - PANGO_DESCENT (rect);
    }

  g_object_unref (layout);

  return TRUE;
}
