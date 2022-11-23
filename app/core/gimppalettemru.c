/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapalettemru.c
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmapalettemru.h"

#include "ligma-intl.h"


#define MAX_N_COLORS 256
#define RGBA_EPSILON 1e-4

enum
{
  COLOR_HISTORY = 1
};


G_DEFINE_TYPE (LigmaPaletteMru, ligma_palette_mru, LIGMA_TYPE_PALETTE)

#define parent_class ligma_palette_mru_parent_class


static void
ligma_palette_mru_class_init (LigmaPaletteMruClass *klass)
{
}

static void
ligma_palette_mru_init (LigmaPaletteMru *palette)
{
}


/*  public functions  */

LigmaData *
ligma_palette_mru_new (const gchar *name)
{
  LigmaPaletteMru *palette;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  palette = g_object_new (LIGMA_TYPE_PALETTE_MRU,
                          "name",      name,
                          "mime-type", "application/x-ligma-palette",
                          NULL);

  return LIGMA_DATA (palette);
}

void
ligma_palette_mru_load (LigmaPaletteMru *mru,
                       GFile          *file)
{
  LigmaPalette *palette;
  GScanner    *scanner;
  GTokenType   token;

  g_return_if_fail (LIGMA_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  palette = LIGMA_PALETTE (mru);

  scanner = ligma_scanner_new_file (file, NULL);
  if (! scanner)
    return;

  g_scanner_scope_add_symbol (scanner, 0, "color-history",
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
                  LigmaRGB color;

                  if (! ligma_scanner_parse_color (scanner, &color))
                    goto end;

                  ligma_palette_add_entry (palette, -1,
                                          _("History Color"), &color);

                  if (ligma_palette_get_n_colors (palette) == MAX_N_COLORS)
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
  ligma_scanner_unref (scanner);
}

void
ligma_palette_mru_save (LigmaPaletteMru *mru,
                       GFile          *file)
{
  LigmaPalette      *palette;
  LigmaConfigWriter *writer;
  GList            *list;

  g_return_if_fail (LIGMA_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  writer = ligma_config_writer_new_from_file (file,
                                             TRUE,
                                             "LIGMA colorrc\n\n"
                                             "This file holds a list of "
                                             "recently used colors.",
                                             NULL);
  if (! writer)
    return;

  palette = LIGMA_PALETTE (mru);

  ligma_config_writer_open (writer, "color-history");

  for (list = palette->colors; list; list = g_list_next (list))
    {
      LigmaPaletteEntry *entry = list->data;
      gchar             buf[4][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_dtostr (buf[0], G_ASCII_DTOSTR_BUF_SIZE, entry->color.r);
      g_ascii_dtostr (buf[1], G_ASCII_DTOSTR_BUF_SIZE, entry->color.g);
      g_ascii_dtostr (buf[2], G_ASCII_DTOSTR_BUF_SIZE, entry->color.b);
      g_ascii_dtostr (buf[3], G_ASCII_DTOSTR_BUF_SIZE, entry->color.a);

      ligma_config_writer_open (writer, "color-rgba");
      ligma_config_writer_printf (writer, "%s %s %s %s",
                                 buf[0], buf[1], buf[2], buf[3]);
      ligma_config_writer_close (writer);
    }

  ligma_config_writer_close (writer);

  ligma_config_writer_finish (writer, "end of colorrc", NULL);
}

void
ligma_palette_mru_add (LigmaPaletteMru *mru,
                      const LigmaRGB  *color)
{
  LigmaPalette *palette;
  GList       *list;

  g_return_if_fail (LIGMA_IS_PALETTE_MRU (mru));
  g_return_if_fail (color != NULL);

  palette = LIGMA_PALETTE (mru);

  /*  is the added color already there?  */
  for (list = ligma_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      LigmaPaletteEntry *entry = list->data;

      if (ligma_rgba_distance (&entry->color, color) < RGBA_EPSILON)
        {
          ligma_palette_move_entry (palette, entry, 0);

          /*  Even though they are nearly the same color, let's make them
           *  exactly equal.
           */
          ligma_palette_set_entry_color (palette, 0, color);

          return;
        }
    }

  if (ligma_palette_get_n_colors (palette) == MAX_N_COLORS)
    {
      ligma_palette_delete_entry (palette,
                                 ligma_palette_get_entry (palette,
                                                         MAX_N_COLORS - 1));
    }

  ligma_palette_add_entry (palette, 0, _("History Color"), color);
}
