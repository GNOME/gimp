/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppalettemru.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimppalettemru.h"

#include "gimp-intl.h"


#define MAX_N_COLORS 256
#define RGBA_EPSILON 1e-4

enum
{
  COLOR_HISTORY = 1,
  COLOR         = 2
};


G_DEFINE_TYPE (GimpPaletteMru, gimp_palette_mru, GIMP_TYPE_PALETTE)

#define parent_class gimp_palette_mru_parent_class


static void
gimp_palette_mru_class_init (GimpPaletteMruClass *klass)
{
}

static void
gimp_palette_mru_init (GimpPaletteMru *palette)
{
}


/*  public functions  */

GimpData *
gimp_palette_mru_new (const gchar *name)
{
  GimpPaletteMru *palette;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  palette = g_object_new (GIMP_TYPE_PALETTE_MRU,
                          "name",      name,
                          "mime-type", "application/x-gimp-palette",
                          NULL);

  return GIMP_DATA (palette);
}

void
gimp_palette_mru_load (GimpPaletteMru *mru,
                       GFile          *file)
{
  GimpPalette *palette;
  GScanner    *scanner;
  GTokenType   token;

  g_return_if_fail (GIMP_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  palette = GIMP_PALETTE (mru);

  scanner = gimp_scanner_new_file (file, NULL);
  if (! scanner)
    return;

  g_scanner_scope_add_symbol (scanner, 0, "color-history",
                              GINT_TO_POINTER (COLOR_HISTORY));
  g_scanner_scope_add_symbol (scanner, 0, "color",
                              GINT_TO_POINTER (COLOR_HISTORY));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GINT_TO_POINTER (COLOR_HISTORY))
            {
              while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
                {
                  GeglColor *color = NULL;

                  if (! gimp_scanner_parse_color (scanner, &color))
                    goto end;

                  gimp_palette_add_entry (palette, -1, _("History Color"), color);
                  g_object_unref (color);

                  if (gimp_palette_get_n_colors (palette) == MAX_N_COLORS)
                    goto end;
                }
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

 end:
  gimp_scanner_unref (scanner);
}

void
gimp_palette_mru_save (GimpPaletteMru *mru,
                       GFile          *file)
{
  GimpPalette      *palette;
  GimpConfigWriter *writer;
  GList            *list;

  g_return_if_fail (GIMP_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  writer = gimp_config_writer_new_from_file (file,
                                             TRUE,
                                             "GIMP colorrc\n\n"
                                             "This file holds a list of "
                                             "recently used colors.",
                                             NULL);
  if (! writer)
    return;

  palette = GIMP_PALETTE (mru);

  gimp_config_writer_open (writer, "color-history");

  for (list = palette->colors; list; list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;
      GeglColor        *color = gegl_color_duplicate (entry->color);
      const gchar      *encoding;
      const Babl       *format = gegl_color_get_format (color);
      const Babl       *space;
      GBytes           *bytes;
      gconstpointer     data;
      gsize             data_length;
      guint8           *profile_data;
      int               profile_length = 0;

      gimp_config_writer_open (writer, "color");

      if (babl_format_is_palette (format))
        {
          guint8 pixel[40];

          /* As a special case, we don't want to serialize
           * palette colors, because they are just too much
           * dependent on external data and cannot be
           * deserialized back safely. So we convert them first.
           */
           format = babl_format_with_space ("R'G'B'A u8", format);
           gegl_color_get_pixel (color, format, pixel);
           gegl_color_set_pixel (color, format, pixel);
        }

      encoding = babl_format_get_encoding (format);
      gimp_config_writer_string (writer, encoding);

      bytes = gegl_color_get_bytes (color, format);
      data  = g_bytes_get_data (bytes, &data_length);

      gimp_config_writer_printf (writer, "%" G_GSIZE_FORMAT, data_length);
      gimp_config_writer_data (writer, data_length, data);

      space = babl_format_get_space (format);
      if (space != babl_space ("sRGB"))
        {
          profile_data = (guint8 *) babl_space_get_icc (space, &profile_length);
          gimp_config_writer_printf (writer, "%u", profile_length);
          if (profile_data)
            gimp_config_writer_data (writer, profile_length, profile_data);
        }
      else
        {
          gimp_config_writer_printf (writer, "%u", profile_length);
        }

      gimp_config_writer_close (writer);
      g_bytes_unref (bytes);
      g_object_unref (color);
    }

  gimp_config_writer_close (writer);

  gimp_config_writer_finish (writer, "end of colorrc", NULL);
}

void
gimp_palette_mru_add (GimpPaletteMru *mru,
                      GeglColor      *color)
{
  GimpPalette *palette;
  GList       *list;

  g_return_if_fail (GIMP_IS_PALETTE_MRU (mru));
  g_return_if_fail (GEGL_IS_COLOR (color));

  palette = GIMP_PALETTE (mru);

  /*  is the added color already there?  */
  for (list = gimp_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;

      if (gimp_color_is_perceptually_identical (entry->color, color))
        {
          gimp_palette_move_entry (palette, entry, 0);

          /*  Even though they are nearly the same color, let's make them
           *  exactly equal.
           */
          gimp_palette_set_entry_color (palette, 0, color, FALSE);
          return;
        }
    }

  if (gimp_palette_get_n_colors (palette) == MAX_N_COLORS)
    {
      gimp_palette_delete_entry (palette,
                                 gimp_palette_get_entry (palette,
                                                         MAX_N_COLORS - 1));
    }

  gimp_palette_add_entry (palette, 0, _("History Color"), color);
}
