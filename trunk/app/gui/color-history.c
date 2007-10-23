/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-history.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "color-history.h"


enum
{
  COLOR_HISTORY = 1
};


static void   color_history_init        (void);
static void   color_history_add_from_rc (GimpRGB *color);


static GimpRGB   color_history[COLOR_HISTORY_SIZE];
static gboolean  color_history_initialized = FALSE;


void
color_history_save (Gimp *gimp)
{
  GimpConfigWriter *writer;
  gchar            *filename;
  gint              i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("colorrc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  writer = gimp_config_writer_new_file (filename,
                                        TRUE,
                                        "GIMP colorrc\n\n"
                                        "This file holds a list of "
                                        "recently used colors.",
                                        NULL);
  g_free (filename);

  if (!writer)
    return;

  if (! color_history_initialized)
    color_history_init ();

  gimp_config_writer_open (writer, "color-history");

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      gchar buf[4][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (buf[0],
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].r);
      g_ascii_formatd (buf[1],
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].g);
      g_ascii_formatd (buf[2],
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].b);
      g_ascii_formatd (buf[3],
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].a);

      gimp_config_writer_open (writer, "color-rgba");
      gimp_config_writer_printf (writer, "%s %s %s %s",
                                 buf[0], buf[1], buf[2], buf[3]);
      gimp_config_writer_close (writer);
    }

  gimp_config_writer_close (writer);

  gimp_config_writer_finish (writer, "end of colorrc", NULL);
}

void
color_history_restore (Gimp *gimp)
{
  gchar      *filename;
  GScanner   *scanner;
  GTokenType  token;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("colorrc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  scanner = gimp_scanner_new_file (filename, NULL);
  g_free (filename);

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
                  GimpRGB color;

                  if (! gimp_scanner_parse_color (scanner, &color))
                    goto error;

                  color_history_add_from_rc (&color);
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

 error:
  gimp_scanner_destroy (scanner);
}

void
color_history_set (gint           index,
                   const GimpRGB *rgb)
{
  g_return_if_fail (index >= 0);
  g_return_if_fail (index < COLOR_HISTORY_SIZE);
  g_return_if_fail (rgb != NULL);

  if (! color_history_initialized)
    color_history_init ();

  color_history[index] = *rgb;
}

void
color_history_get (gint     index,
                   GimpRGB *rgb)
{
  g_return_if_fail (index >= 0);
  g_return_if_fail (index < COLOR_HISTORY_SIZE);
  g_return_if_fail (rgb != NULL);

  if (! color_history_initialized)
    color_history_init ();

  *rgb = color_history[index];
}

gint
color_history_add (const GimpRGB *rgb)
{
  gint shift_begin = -1;
  gint i, j;

  g_return_val_if_fail (rgb != NULL, 0);

  if (! color_history_initialized)
    color_history_init ();

  /*  is the added color already there?  */
  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      if (gimp_rgba_distance (&color_history[i], rgb) < 0.0001)
        {
          shift_begin = i;

          goto doit;
        }
    }

  /*  if not, are there two equal colors?  */
  if (shift_begin == -1)
    {
      for (i = 0; i < COLOR_HISTORY_SIZE; i++)
        {
          for (j = i + 1; j < COLOR_HISTORY_SIZE; j++)
            {
              if (gimp_rgba_distance (&color_history[i],
                                      &color_history[j]) < 0.0001)
                {
                  shift_begin = i;

                  goto doit;
                }
            }
        }
    }

  /*  if not, shift them all  */
  if (shift_begin == -1)
    shift_begin = COLOR_HISTORY_SIZE - 1;

 doit:

  for (i = shift_begin; i > 0; i--)
    color_history[i] = color_history[i - 1];

  color_history[0] = *rgb;

  return shift_begin;
}


/*  private functions  */

static void
color_history_init (void)
{
  gint i;

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    gimp_rgba_set (&color_history[i], 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

  color_history_initialized = TRUE;
}

static void
color_history_add_from_rc (GimpRGB *color)
{
  static gint index = 0;

  if (! color_history_initialized)
    color_history_init ();

  if (color && index < COLOR_HISTORY_SIZE)
    {
      color_history[index++] = *color;
    }
}
