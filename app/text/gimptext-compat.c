/* The GIMP -- an image manipulation program
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
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer-floating-sel.h"

#include "gimptext.h"
#include "gimptext-compat.h"
#include "gimptextlayer.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


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
  GimpRGB    color;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  if (border < 0)
    border = 0;

  gimp_context_get_foreground (gimp_get_current_context (gimage->gimp),
                               &color);

  gtext = GIMP_TEXT (g_object_new (GIMP_TYPE_TEXT,
                                   "text",   text,
                                   "font",   fontname,
                                   "border", border,
				   "color",  &color,
                                   NULL));

  /*  if font-size is < 0, it is taken from the font name */
  gtext->font_size = -1.0;

  layer = gimp_text_layer_new (gimage, gtext);

  g_object_unref (gtext);

  if (!layer)
    return NULL;

  /*  Start a group undo  */
  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TEXT,
                               _("Text"));

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
  gimp_image_undo_group_end (gimage);

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
  PangoRectangle        ink_rect;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  font_desc = pango_font_description_from_string (fontname);
  if (!font_desc)
    return FALSE;

  /* FIXME: resolution */
  context = pango_ft2_get_context (72.0, 72.0);

  layout = pango_layout_new (context);

  g_object_unref (context);  

  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, &ink_rect, NULL);

  if (width)
    *width = ink_rect.width;
  if (height)
    *height = ink_rect.height;
  if (ascent)
    *ascent = -ink_rect.y;
  if (descent)
    *descent = ink_rect.height + ink_rect.y;

  g_object_unref (layout);

  return TRUE;
}
