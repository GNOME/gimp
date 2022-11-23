/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligma-utils.h"
#include "ligmapalette.h"
#include "ligmapalette-load.h"

#include "ligma-intl.h"


GList *
ligma_palette_load (LigmaContext   *context,
                   GFile         *file,
                   GInputStream  *input,
                   GError       **error)
{
  LigmaPalette      *palette = NULL;
  LigmaPaletteEntry *entry;
  GDataInputStream *data_input;
  gchar            *str;
  gsize             str_len;
  gchar            *tok;
  gint              r, g, b;
  gint              linenum;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data_input = g_data_input_stream_new (input);

  r = g = b = 0;

  linenum = 1;
  str_len = 1024;
  str = ligma_data_input_stream_read_line_always (data_input, &str_len,
                                                 NULL, error);
  if (! str)
    goto failed;

  if (! g_str_has_prefix (str, "LIGMA Palette"))
    {
      g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                   _("Missing magic header."));
      g_free (str);
      goto failed;
    }

  g_free (str);

  palette = g_object_new (LIGMA_TYPE_PALETTE,
                          "mime-type", "application/x-ligma-palette",
                          NULL);

  linenum++;
  str_len = 1024;
  str = ligma_data_input_stream_read_line_always (data_input, &str_len,
                                                 NULL, error);
  if (! str)
    goto failed;

  if (g_str_has_prefix (str, "Name: "))
    {
      gchar *utf8;

      utf8 = ligma_any_to_utf8 (g_strstrip (str + strlen ("Name: ")), -1,
                               _("Invalid UTF-8 string in palette file '%s'"),
                               ligma_file_get_utf8_name (file));
      ligma_object_take_name (LIGMA_OBJECT (palette), utf8);
      g_free (str);

      linenum++;
      str_len = 1024;
      str = ligma_data_input_stream_read_line_always (data_input, &str_len,
                                                     NULL, error);
      if (! str)
        goto failed;

      if (g_str_has_prefix (str, "Columns: "))
        {
          gint columns;

          if (! ligma_ascii_strtoi (g_strstrip (str + strlen ("Columns: ")),
                                   NULL, 10, &columns))
            {
              g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                           _("Invalid column count."));
              g_free (str);
              goto failed;
            }

          if (columns < 0 || columns > 256)
            {
              g_message (_("Reading palette file '%s': "
                           "Invalid number of columns in line %d. "
                           "Using default value."),
                         ligma_file_get_utf8_name (file), linenum);
              columns = 0;
            }

          ligma_palette_set_columns (palette, columns);
          g_free (str);

          linenum++;
          str_len = 1024;
          str = ligma_data_input_stream_read_line_always (data_input, &str_len,
                                                         NULL, error);
          if (! str)
            goto failed;
        }
    }
  else /* old palette format */
    {
      ligma_object_take_name (LIGMA_OBJECT (palette),
                             g_path_get_basename (ligma_file_get_utf8_name (file)));
    }

  while (str)
    {
      GError *my_error = NULL;

      if (str[0] != '#' && str[0] != '\0')
        {
          tok = strtok (str, " \t");
          if (tok)
            r = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing RED component in line %d."),
                       ligma_file_get_utf8_name (file), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            g = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing GREEN component in line %d."),
                       ligma_file_get_utf8_name (file), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            b = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing BLUE component in line %d."),
                       ligma_file_get_utf8_name (file), linenum);

          /* optional name */
          tok = strtok (NULL, "\n");

          if (r < 0 || r > 255 ||
              g < 0 || g > 255 ||
              b < 0 || b > 255)
            g_message (_("Reading palette file '%s': "
                         "RGB value out of range in line %d."),
                       ligma_file_get_utf8_name (file), linenum);

          /* don't call ligma_palette_add_entry here, it's rather inefficient */
          entry = g_slice_new0 (LigmaPaletteEntry);

          ligma_rgba_set_uchar (&entry->color,
                               (guchar) r,
                               (guchar) g,
                               (guchar) b,
                               255);

          entry->name     = g_strdup (tok ? tok : _("Untitled"));

          palette->colors = g_list_prepend (palette->colors, entry);
          palette->n_colors++;
        }

      g_free (str);

      linenum++;
      str_len = 1024;
      str = g_data_input_stream_read_line (data_input, &str_len,
                                           NULL, &my_error);
      if (! str && my_error)
        {
          g_message (_("Reading palette file '%s': "
                       "Read %d colors from truncated file: %s"),
                     ligma_file_get_utf8_name (file),
                     g_list_length (palette->colors),
                     my_error->message);
          g_clear_error (&my_error);
        }
    }

  palette->colors = g_list_reverse (palette->colors);

  g_object_unref (data_input);

  return g_list_prepend (NULL, palette);

 failed:

  g_object_unref (data_input);

  if (palette)
    g_object_unref (palette);

  g_prefix_error (error, _("In line %d of palette file: "), linenum);

  return NULL;
}

GList *
ligma_palette_load_act (LigmaContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  LigmaPalette *palette;
  gchar       *palette_name;
  guchar       color_bytes[3];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name = g_path_get_basename (ligma_file_get_utf8_name (file));
  palette = LIGMA_PALETTE (ligma_palette_new (context, palette_name));
  g_free (palette_name);

  while (g_input_stream_read_all (input, color_bytes, sizeof (color_bytes),
                                  &bytes_read, NULL, NULL) &&
         bytes_read == sizeof (color_bytes))
    {
      LigmaRGB color;

      ligma_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      ligma_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
ligma_palette_load_riff (LigmaContext   *context,
                        GFile         *file,
                        GInputStream  *input,
                        GError       **error)
{
  LigmaPalette *palette;
  gchar       *palette_name;
  guchar       color_bytes[4];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name = g_path_get_basename (ligma_file_get_utf8_name (file));
  palette = LIGMA_PALETTE (ligma_palette_new (context, palette_name));
  g_free (palette_name);

  if (! g_seekable_seek (G_SEEKABLE (input), 28, G_SEEK_SET, NULL, error))
    {
      g_object_unref (palette);
      return NULL;
    }

  while (g_input_stream_read_all (input, color_bytes, sizeof (color_bytes),
                                  &bytes_read, NULL, NULL) &&
         bytes_read == sizeof (color_bytes))
    {
      LigmaRGB color;

      ligma_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      ligma_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
ligma_palette_load_psp (LigmaContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  LigmaPalette *palette;
  gchar       *palette_name;
  guchar       color_bytes[4];
  gint         number_of_colors;
  gsize        bytes_read;
  gint         i, j;
  gboolean     color_ok;
  gchar        buffer[4096];
  /*Maximum valid file size: 256 * 4 * 3 + 256 * 2  ~= 3650 bytes */
  gchar      **lines;
  gchar      **ascii_colors;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name = g_path_get_basename (ligma_file_get_utf8_name (file));
  palette = LIGMA_PALETTE (ligma_palette_new (context, palette_name));
  g_free (palette_name);

  if (! g_seekable_seek (G_SEEKABLE (input), 16, G_SEEK_SET, NULL, error))
    {
      g_object_unref (palette);
      return NULL;
    }

  if (! g_input_stream_read_all (input, buffer, sizeof (buffer) - 1,
                                 &bytes_read, NULL, error))
    {
      g_object_unref (palette);
      return NULL;
    }

  buffer[bytes_read] = '\0';

  lines = g_strsplit (buffer, "\x0d\x0a", -1);

  number_of_colors = atoi (lines[0]);

  for (i = 0; i < number_of_colors; i++)
    {
      if (lines[i + 1] == NULL)
        {
          g_printerr ("Premature end of file reading %s.",
                      ligma_file_get_utf8_name (file));
          break;
        }

      ascii_colors = g_strsplit (lines[i + 1], " ", 3);
      color_ok = TRUE;

      for (j = 0 ; j < 3; j++)
        {
          if (ascii_colors[j] == NULL)
            {
              g_printerr ("Corrupted palette file %s.",
                          ligma_file_get_utf8_name (file));
              color_ok = FALSE;
              break;
            }

          color_bytes[j] = atoi (ascii_colors[j]);
        }

      if (color_ok)
        {
          LigmaRGB color;

          ligma_rgba_set_uchar (&color,
                               color_bytes[0],
                               color_bytes[1],
                               color_bytes[2],
                               255);
          ligma_palette_add_entry (palette, -1, NULL, &color);
        }

      g_strfreev (ascii_colors);
    }

  g_strfreev (lines);

  return g_list_prepend (NULL, palette);
}

GList *
ligma_palette_load_aco (LigmaContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  LigmaPalette *palette;
  gchar       *palette_name;
  gint         format_version;
  gint         number_of_colors;
  gint         i;
  gchar        header[4];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! g_input_stream_read_all (input, header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      g_prefix_error (error,
                      _("Could not read header from palette file '%s': "),
                      ligma_file_get_utf8_name (file));
      return NULL;
    }

  palette_name = g_path_get_basename (ligma_file_get_utf8_name (file));
  palette = LIGMA_PALETTE (ligma_palette_new (context, palette_name));
  g_free (palette_name);

  format_version   = header[1] + (header[0] << 8);
  number_of_colors = header[3] + (header[2] << 8);

  for (i = 0; i < number_of_colors; i++)
    {
      gchar     color_info[10];
      gint      color_space;
      gint      w, x, y, z;
      gboolean  color_ok = FALSE;
      LigmaRGB   color;
      GError   *my_error = NULL;

      if (! g_input_stream_read_all (input, color_info, sizeof (color_info),
                                     &bytes_read, NULL, &my_error) ||
          bytes_read != sizeof (color_info))
        {
          if (palette->colors)
            {
              g_message (_("Reading palette file '%s': "
                           "Read %d colors from truncated file: %s"),
                         ligma_file_get_utf8_name (file),
                         g_list_length (palette->colors),
                         my_error ?
                         my_error->message : _("Premature end of file."));
              g_clear_error (&my_error);
              break;
            }

          g_propagate_error (error, my_error);
          g_object_unref (palette);

          return NULL;
        }

      color_space = color_info[1] + (color_info[0] << 8);

      w = (guchar) color_info[3] + ((guchar) color_info[2] << 8);
      x = (guchar) color_info[5] + ((guchar) color_info[4] << 8);
      y = (guchar) color_info[7] + ((guchar) color_info[6] << 8);
      z = (guchar) color_info[9] + ((guchar) color_info[8] << 8);

      if (color_space == 0) /* RGB */
        {
          gdouble R = ((gdouble) w) / 65536.0;
          gdouble G = ((gdouble) x) / 65536.0;
          gdouble B = ((gdouble) y) / 65536.0;

          ligma_rgba_set (&color, R, G, B, 1.0);

          color_ok = TRUE;
        }
      else if (color_space == 1) /* HSV */
        {
          LigmaHSV hsv;

          gdouble H = ((gdouble) w) / 65536.0;
          gdouble S = ((gdouble) x) / 65536.0;
          gdouble V = ((gdouble) y) / 65536.0;

          ligma_hsva_set (&hsv, H, S, V, 1.0);
          ligma_hsv_to_rgb (&hsv, &color);

          color_ok = TRUE;
        }
      else if (color_space == 2) /* CMYK */
        {
          LigmaCMYK cmyk;

          gdouble C = 1.0 - (((gdouble) w) / 65536.0);
          gdouble M = 1.0 - (((gdouble) x) / 65536.0);
          gdouble Y = 1.0 - (((gdouble) y) / 65536.0);
          gdouble K = 1.0 - (((gdouble) z) / 65536.0);

          ligma_cmyka_set (&cmyk, C, M, Y, K, 1.0);
          ligma_cmyk_to_rgb (&cmyk, &color);

          color_ok = TRUE;
        }
      else if (color_space == 8) /* Grayscale */
        {
          gdouble K = 1.0 - (((gdouble) w) / 10000.0);

          ligma_rgba_set (&color, K, K, K, 1.0);

          color_ok = TRUE;
        }
      else if (color_space == 9) /* Wide? CMYK */
        {
          LigmaCMYK cmyk;

          gdouble C = 1.0 - (((gdouble) w) / 10000.0);
          gdouble M = 1.0 - (((gdouble) x) / 10000.0);
          gdouble Y = 1.0 - (((gdouble) y) / 10000.0);
          gdouble K = 1.0 - (((gdouble) z) / 10000.0);

          ligma_cmyka_set (&cmyk, C, M, Y, K, 1.0);
          ligma_cmyk_to_rgb (&cmyk, &color);

          color_ok = TRUE;
        }
      else
        {
          g_printerr ("Unsupported color space (%d) in ACO file %s\n",
                      color_space, ligma_file_get_utf8_name (file));
        }

      if (format_version == 2)
        {
          gchar format2_preamble[4];
          gint  number_of_chars;

          if (! g_input_stream_read_all (input,
                                         format2_preamble,
                                         sizeof (format2_preamble),
                                         &bytes_read, NULL, error) ||
              bytes_read != sizeof (format2_preamble))
            {
              g_object_unref (palette);
              return NULL;
            }

          number_of_chars = format2_preamble[3] + (format2_preamble[2] << 8);

          if (! g_seekable_seek (G_SEEKABLE (input), number_of_chars * 2,
                                 G_SEEK_SET, NULL, error))
            {
              g_object_unref (palette);
              return NULL;
            }
        }

      if (color_ok)
        ligma_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}


GList *
ligma_palette_load_css (LigmaContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  LigmaPalette      *palette;
  GDataInputStream *data_input;
  gchar            *name;
  GRegex           *regex;
  gchar            *buf;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  regex = g_regex_new (".*color.*:(?P<param>.*);", G_REGEX_CASELESS, 0, error);
  if (! regex)
    return NULL;

  name = g_path_get_basename (ligma_file_get_utf8_name (file));
  palette = LIGMA_PALETTE (ligma_palette_new (context, name));
  g_free (name);

  data_input = g_data_input_stream_new (input);

  do
    {
      gsize  buf_len = 1024;

      buf = g_data_input_stream_read_line (data_input, &buf_len, NULL, NULL);

      if (buf)
        {
          GMatchInfo *matches;

          if (g_regex_match (regex, buf, 0, &matches))
            {
              LigmaRGB  color;
              gchar   *word = g_match_info_fetch_named (matches, "param");

              if (ligma_rgb_parse_css (&color, word, -1))
                {
                  if (! ligma_palette_find_entry (palette, &color, NULL))
                    {
                      ligma_palette_add_entry (palette, -1, NULL, &color);
                    }
                }

              g_free (word);
            }
          g_match_info_free (matches);
          g_free (buf);
        }
    }
  while (buf);

  g_regex_unref (regex);
  g_object_unref (data_input);

  return g_list_prepend (NULL, palette);
}

LigmaPaletteFileFormat
ligma_palette_load_detect_format (GFile        *file,
                                 GInputStream *input)
{
  LigmaPaletteFileFormat format = LIGMA_PALETTE_FILE_FORMAT_UNKNOWN;
  gchar                 header[16];
  gsize                 bytes_read;

  if (g_input_stream_read_all (input, &header, sizeof (header),
                               &bytes_read, NULL, NULL) &&
      bytes_read == sizeof (header))
    {
      if (g_str_has_prefix (header + 0, "RIFF") &&
          g_str_has_prefix (header + 8, "PAL data"))
        {
          format = LIGMA_PALETTE_FILE_FORMAT_RIFF_PAL;
        }
      else if (g_str_has_prefix (header, "LIGMA Palette"))
        {
          format = LIGMA_PALETTE_FILE_FORMAT_GPL;
        }
      else if (g_str_has_prefix (header, "JASC-PAL"))
        {
          format = LIGMA_PALETTE_FILE_FORMAT_PSP_PAL;
        }
    }

  if (format == LIGMA_PALETTE_FILE_FORMAT_UNKNOWN)
    {
      gchar *lower = g_ascii_strdown (ligma_file_get_utf8_name (file), -1);

      if (g_str_has_suffix (lower, ".aco"))
        {
          format = LIGMA_PALETTE_FILE_FORMAT_ACO;
        }
      else if (g_str_has_suffix (lower, ".css"))
        {
          format = LIGMA_PALETTE_FILE_FORMAT_CSS;
        }

      g_free (lower);
    }

  if (format == LIGMA_PALETTE_FILE_FORMAT_UNKNOWN)
    {
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);

      if (info)
        {
          goffset size = g_file_info_get_size (info);

          if (size == 768)
            format = LIGMA_PALETTE_FILE_FORMAT_ACT;

          g_object_unref (info);
        }
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  return format;
}
