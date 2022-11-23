/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextTag
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"

#include "widgets-types.h"

#include "ligmatexttag.h"


gint
ligma_text_tag_get_size (GtkTextTag *tag)
{
  gint size;

  g_return_val_if_fail (GTK_IS_TEXT_TAG (tag), 0);

  g_object_get (tag,
                LIGMA_TEXT_PROP_NAME_SIZE, &size,
                NULL);

  return size;
}

gint
ligma_text_tag_get_baseline (GtkTextTag *tag)
{
  gint baseline;

  g_object_get (tag,
                LIGMA_TEXT_PROP_NAME_BASELINE, &baseline,
                NULL);

  return baseline;
}

gint
ligma_text_tag_get_kerning (GtkTextTag *tag)
{
  gint kerning;

  g_object_get (tag,
                LIGMA_TEXT_PROP_NAME_KERNING, &kerning,
                NULL);

  return kerning;
}

gchar *
ligma_text_tag_get_font (GtkTextTag *tag)
{
  gchar *font;

  g_object_get (tag,
                LIGMA_TEXT_PROP_NAME_FONT, &font,
                NULL);

  return font;
}

gboolean
ligma_text_tag_get_fg_color (GtkTextTag *tag,
                            LigmaRGB    *color)
{
  GdkRGBA  *rgba;
  gboolean  set;

  g_object_get (tag,
                "foreground-set",             &set,
                LIGMA_TEXT_PROP_NAME_FG_COLOR, &rgba,
                NULL);

  ligma_rgb_set (color, rgba->red, rgba->green, rgba->blue);

  gdk_rgba_free (rgba);

  return set;
}

gboolean
ligma_text_tag_get_bg_color (GtkTextTag *tag,
                            LigmaRGB    *color)
{
  GdkRGBA  *rgba;
  gboolean  set;

  g_object_get (tag,
                "background-set",             &set,
                LIGMA_TEXT_PROP_NAME_BG_COLOR, &rgba,
                NULL);

  ligma_rgb_set (color, rgba->red, rgba->green, rgba->blue);

  gdk_rgba_free (rgba);

  return set;
}
