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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppalette.h"
#include "gimppalette-load.h"

#include "gimp-intl.h"


GList *
gimp_palette_load (const gchar  *filename,
                   GError      **error)
{
  GimpPalette      *palette;
  GimpPaletteEntry *entry;
  gchar             str[1024];
  gchar            *tok;
  FILE             *file;
  gint              r, g, b;
  gint              linenum;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  r = g = b = 0;

  file = g_fopen (filename, "r");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  linenum = 0;

  fread (str, 13, 1, file);
  str[13] = '\0';
  linenum++;
  if (strcmp (str, "GIMP Palette\n"))
    {
      /* bad magic, but maybe it has \r\n at the end of lines? */
      if (!strcmp (str, "GIMP Palette\r"))
        g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                     _("Fatal parse error in palette file '%s': "
                       "Missing magic header.\n"
                       "Does this file need converting from DOS?"),
                     gimp_filename_to_utf8 (filename));
      else
        g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                     _("Fatal parse error in palette file '%s': "
                       "Missing magic header."),
                     gimp_filename_to_utf8 (filename));

      fclose (file);

      return NULL;
    }

  palette = g_object_new (GIMP_TYPE_PALETTE,
                          "mime-type", "application/x-gimp-palette",
                          NULL);

  if (! fgets (str, sizeof (str), file))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in palette file '%s': "
                     "Read error in line %d."),
                   gimp_filename_to_utf8 (filename), linenum);
      fclose (file);
      g_object_unref (palette);
      return NULL;
    }

  linenum++;

  if (! strncmp (str, "Name: ", strlen ("Name: ")))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (&str[strlen ("Name: ")], -1,
                               _("Invalid UTF-8 string in palette file '%s'"),
                               gimp_filename_to_utf8 (filename));

      gimp_object_take_name (GIMP_OBJECT (palette), g_strstrip (utf8));

      if (! fgets (str, sizeof (str), file))
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in palette file '%s': "
                         "Read error in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);
          fclose (file);
          g_object_unref (palette);
          return NULL;
        }

      linenum++;

      if (! strncmp (str, "Columns: ", strlen ("Columns: ")))
        {
          gint columns;

          columns = atoi (g_strstrip (&str[strlen ("Columns: ")]));

          if (columns < 0 || columns > 256)
            {
              g_message (_("Reading palette file '%s': "
                           "Invalid number of columns in line %d. "
                           "Using default value."),
                         gimp_filename_to_utf8 (filename), linenum);
              columns = 0;
            }

          palette->n_columns = columns;

          if (! fgets (str, sizeof (str), file))
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Fatal parse error in palette file '%s': "
                             "Read error in line %d."),
                           gimp_filename_to_utf8 (filename), linenum);
              fclose (file);
              g_object_unref (palette);
              return NULL;
            }

          linenum++;
        }
    }
  else /* old palette format */
    {
      gimp_object_take_name (GIMP_OBJECT (palette),
                             g_filename_display_basename (filename));
    }

  while (! feof (file))
    {
      if (str[0] != '#')
        {
          tok = strtok (str, " \t");
          if (tok)
            r = atoi (tok);
          else
            /* maybe we should just abort? */
            g_message (_("Reading palette file '%s': "
                         "Missing RED component in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            g = atoi (tok);
          else
            g_message (_("Reading palette '%s': "
                         "Missing GREEN component in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            b = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing BLUE component in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);

          /* optional name */
          tok = strtok (NULL, "\n");

          if (r < 0 || r > 255 ||
              g < 0 || g > 255 ||
              b < 0 || b > 255)
            g_message (_("Reading palette file '%s': "
                         "RGB value out of range in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);

          /* don't call gimp_palette_add_entry here, it's rather inefficient */
          entry = g_new0 (GimpPaletteEntry, 1);

          gimp_rgba_set_uchar (&entry->color,
                               (guchar) r,
                               (guchar) g,
                               (guchar) b,
                               255);

          entry->name     = g_strdup (tok ? tok : _("Untitled"));
          entry->position = palette->n_colors;

          palette->colors = g_list_prepend (palette->colors, entry);
          palette->n_colors++;
        }

      if (! fgets (str, sizeof (str), file))
        {
          if (feof (file))
            break;

          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in palette file '%s': "
                         "Read error in line %d."),
                       gimp_filename_to_utf8 (filename), linenum);
          fclose (file);
          g_object_unref (palette);
          return NULL;
        }

      linenum++;
    }

  fclose (file);

  palette->colors = g_list_reverse (palette->colors);

  return g_list_prepend (NULL, palette);
}
