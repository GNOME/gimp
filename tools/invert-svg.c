/* invert-svg-grey.c
 * Copyright (C) 2016 Jehan
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

#include <gio/gio.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>

/* This tool inverts grey colors in a SVG image.
 * Non-grey colors are not touched, since they are considered
 * exceptions that we would want to keep the same (for instance
 * Red, Blue or Green channel representations).
 *
 * It is not based off XML knowledge since colors could appear in
 * various fields. Instead we just assume that a color has the XML
 * format "#RRGGBB" and we use regular expression to update these.
 */
static gboolean
invert_rgb_color (const GMatchInfo *info,
                  GString          *res,
                  gpointer          data)
{
  gchar *match;
  gchar *inverted;
  gint   value;

  /* We only invert grey colors, so we just need the first channel. */
  match = g_match_info_fetch (info, 1);
  value = strtol (match, NULL, 16);
  value = 255 - value;
  inverted = g_strdup_printf ("#%02x%02x%02x",
                              value, value, value);

  g_string_append (res, inverted);
  g_free (inverted);
  g_free (match);

  return FALSE;
}

int main (int argc, char **argv)
{
  gchar  *input;
  gchar  *output;
  GFile  *file;
  gchar  *contents;
  gchar  *replaced;
  GRegex *regex;
  gint    retval = 0;

  if (argc != 3)
    {
      g_fprintf (stderr, "Usage: invert-svg svg-image inverted-svg-output\n");
      return 1;
    }
  input  = argv[1];
  output = argv[2];

  file = g_file_new_for_path (input);
  if (! g_file_load_contents (file, NULL, &contents, NULL, NULL, NULL))
    {
      g_fprintf (stderr,
                 "Error: invert-svg could not load contents of file %s.\n",
                 input);
      g_object_unref (file);
      return 1;
    }
  g_object_unref (file);

  /* Replace grey colors only. */
  regex = g_regex_new ("#([0-9a-fA-F]{2}){3}\\b", 0, 0, NULL);
  replaced = g_regex_replace_eval (regex, contents, -1, 0, 0,
                                   invert_rgb_color, NULL, NULL);

  file = g_file_new_for_path (output);
  if (! g_file_replace_contents (file, replaced, strlen (replaced),
                                 NULL, FALSE,
                                 G_FILE_CREATE_REPLACE_DESTINATION,
                                 NULL, NULL, NULL))
    {
      g_fprintf (stderr,
                 "Error: invert-svg could not save file %s.\n",
                 output);
      retval = 1;
    }

  g_object_unref (file);
  g_free (contents);
  g_free (replaced);
  g_regex_unref (regex);

  return retval;
}
