/* GIMP - The GNU Image Manipulation Program
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

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

  file = g_fopen (filename, "rb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  linenum = 1;
  if (! fgets (str, sizeof (str), file))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in palette file '%s': "
                     "Read error in line %d."),
                   gimp_filename_to_utf8 (filename), linenum);
      fclose (file);
      return NULL;
    }

  if (! g_str_has_prefix (str, "GIMP Palette"))
    {
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

  linenum++;
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

  if (g_str_has_prefix (str, "Name: "))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (g_strstrip (str + strlen ("Name: ")), -1,
                               _("Invalid UTF-8 string in palette file '%s'"),
                               gimp_filename_to_utf8 (filename));
      gimp_object_take_name (GIMP_OBJECT (palette), utf8);

      linenum++;
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

      if (g_str_has_prefix (str, "Columns: "))
        {
          gint columns;

          columns = atoi (g_strstrip (str + strlen ("Columns: ")));

          if (columns < 0 || columns > 256)
            {
              g_message (_("Reading palette file '%s': "
                           "Invalid number of columns in line %d. "
                           "Using default value."),
                         gimp_filename_to_utf8 (filename), linenum);
              columns = 0;
            }

          palette->n_columns = columns;

          linenum++;
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
          entry = g_slice_new0 (GimpPaletteEntry);

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

      linenum++;
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
    }

  fclose (file);

  palette->colors = g_list_reverse (palette->colors);

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_act (const gchar  *filename,
                       GError      **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  gint         fd;
  guchar       color_bytes[4];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);
  if (! fd)
    {
      g_set_error (error,
                   G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  palette_name = g_filename_display_basename (filename);
  palette = GIMP_PALETTE (gimp_palette_new (palette_name));
  g_free (palette_name);

  while (read (fd, color_bytes, 3) == 3)
    {
      GimpRGB color;

      gimp_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  close (fd);

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_riff (const gchar  *filename,
                        GError      **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  gint         fd;
  guchar       color_bytes[4];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);
  if (! fd)
    {
      g_set_error (error,
                   G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  palette_name = g_filename_display_basename (filename);
  palette = GIMP_PALETTE (gimp_palette_new (palette_name));
  g_free (palette_name);

  lseek (fd, 28, SEEK_SET);
  while (read (fd,
               color_bytes, sizeof (color_bytes)) == sizeof (color_bytes))
    {
      GimpRGB color;

      gimp_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  close (fd);

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_psp (const gchar  *filename,
                       GError      **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  gint         fd;
  guchar       color_bytes[4];
  gint         number_of_colors;
  gint         data_size;
  gint         i, j;
  gboolean     color_ok;
  gchar        buffer[4096];
  /*Maximum valid file size: 256 * 4 * 3 + 256 * 2  ~= 3650 bytes */
  gchar      **lines;
  gchar      **ascii_colors;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);
  if (! fd)
    {
      g_set_error (error,
                   G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  palette_name = g_filename_display_basename (filename);
  palette = GIMP_PALETTE (gimp_palette_new (palette_name));
  g_free (palette_name);

  lseek (fd, 16, SEEK_SET);
  data_size = read (fd, buffer, sizeof (buffer) - 1);
  buffer[data_size] = '\0';

  lines = g_strsplit (buffer, "\x0d\x0a", -1);

  number_of_colors = atoi (lines[0]);

  for (i = 0; i < number_of_colors; i++)
    {
      if (lines[i + 1] == NULL)
        {
          g_printerr ("Premature end of file reading %s.",
                      gimp_filename_to_utf8 (filename));
          break;
        }

      ascii_colors = g_strsplit (lines[i + 1], " ", 3);
      color_ok = TRUE;

      for (j = 0 ; j < 3; j++)
        {
          if (ascii_colors[j] == NULL)
            {
              g_printerr ("Corrupted palette file %s.",
                          gimp_filename_to_utf8 (filename));
              color_ok = FALSE;
              break;
            }

          color_bytes[j] = atoi (ascii_colors[j]);
        }

      if (color_ok)
        {
          GimpRGB color;

          gimp_rgba_set_uchar (&color,
                               color_bytes[0],
                               color_bytes[1],
                               color_bytes[2],
                               255);
          gimp_palette_add_entry (palette, -1, NULL, &color);
        }

      g_strfreev (ascii_colors);
    }

  g_strfreev (lines);

  close (fd);

  return g_list_prepend (NULL, palette);
}

GimpPaletteFileFormat
gimp_palette_load_detect_format (const gchar *filename)
{
  GimpPaletteFileFormat format = GIMP_PALETTE_FILE_FORMAT_UNKNOWN;
  gint                  fd;
  gchar                 header[16];
  struct stat           file_stat;

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);
  if (fd)
    {
      if (read (fd, header, sizeof (header)) == sizeof (header))
        {
          if (g_str_has_prefix (header + 0, "RIFF") &&
              g_str_has_prefix (header + 8, "PAL data"))
             {
              format = GIMP_PALETTE_FILE_FORMAT_RIFF_PAL;
            }
          else if (g_str_has_prefix (header, "GIMP Palette"))
            {
              format = GIMP_PALETTE_FILE_FORMAT_GPL;
            }
          else if (g_str_has_prefix (header, "JASC-PAL"))
            {
              format = GIMP_PALETTE_FILE_FORMAT_PSP_PAL;
            }
        }

      if (fstat (fd, &file_stat) >= 0)
        {
          if (file_stat.st_size == 768)
            format = GIMP_PALETTE_FILE_FORMAT_ACT;
        }

      close (fd);
    }

  return format;
}

