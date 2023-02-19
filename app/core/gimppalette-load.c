/* GIMP - The GNU Image Manipulation Program
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

#include <archive.h>
#include <archive_entry.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimp-utils.h"
#include "gimppalette.h"
#include "gimppalette-load.h"

#include "gimp-intl.h"

typedef struct
{
  GimpColorProfile *profile;
  gchar            *id;
} SwatchBookerColorProfile;

typedef struct
{
  GimpPalette *palette;
  gint         position;
  gchar       *palette_name;
  gchar       *color_model;
  gchar       *color_space;
  GList       *embedded_profiles;
  gboolean     in_color_tag;
  gboolean     in_book_tag;
  gboolean     copy_name;
  gboolean     copy_values;
  gint         sorted_position;
} SwatchBookerData;

/* SwatchBooker XML parser functions */
static void swatchbooker_load_start_element (GMarkupParseContext *context,
                                             const gchar         *element_name,
                                             const gchar        **attribute_names,
                                             const gchar        **attribute_values,
                                             gpointer             user_data,
                                             GError             **error);
static void swatchbooker_load_end_element   (GMarkupParseContext *context,
                                             const gchar         *element_name,
                                             gpointer             user_data,
                                             GError             **error);
static void swatchbooker_load_text          (GMarkupParseContext *context,
                                             const gchar         *text,
                                             gsize                text_len,
                                             gpointer             user_data,
                                             GError             **error);


static gchar * gimp_palette_load_ase_block_name (GInputStream  *input,
                                                 goffset        file_size,
                                                 GError       **error);


GList *
gimp_palette_load (GimpContext   *context,
                   GFile         *file,
                   GInputStream  *input,
                   GError       **error)
{
  GimpPalette      *palette = NULL;
  GimpPaletteEntry *entry;
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
  str = gimp_data_input_stream_read_line_always (data_input, &str_len,
                                                 NULL, error);
  if (! str)
    goto failed;

  if (! g_str_has_prefix (str, "GIMP Palette"))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Missing magic header."));
      g_free (str);
      goto failed;
    }

  g_free (str);

  palette = g_object_new (GIMP_TYPE_PALETTE,
                          "mime-type", "application/x-gimp-palette",
                          NULL);

  linenum++;
  str_len = 1024;
  str = gimp_data_input_stream_read_line_always (data_input, &str_len,
                                                 NULL, error);
  if (! str)
    goto failed;

  if (g_str_has_prefix (str, "Name: "))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (g_strstrip (str + strlen ("Name: ")), -1,
                               _("Invalid UTF-8 string in palette file '%s'"),
                               gimp_file_get_utf8_name (file));
      gimp_object_take_name (GIMP_OBJECT (palette), utf8);
      g_free (str);

      linenum++;
      str_len = 1024;
      str = gimp_data_input_stream_read_line_always (data_input, &str_len,
                                                     NULL, error);
      if (! str)
        goto failed;

      if (g_str_has_prefix (str, "Columns: "))
        {
          gint columns;

          if (! gimp_ascii_strtoi (g_strstrip (str + strlen ("Columns: ")),
                                   NULL, 10, &columns))
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Invalid column count."));
              g_free (str);
              goto failed;
            }

          if (columns < 0 || columns > 256)
            {
              g_message (_("Reading palette file '%s': "
                           "Invalid number of columns in line %d. "
                           "Using default value."),
                         gimp_file_get_utf8_name (file), linenum);
              columns = 0;
            }

          gimp_palette_set_columns (palette, columns);
          g_free (str);

          linenum++;
          str_len = 1024;
          str = gimp_data_input_stream_read_line_always (data_input, &str_len,
                                                         NULL, error);
          if (! str)
            goto failed;
        }
    }
  else /* old palette format */
    {
      gimp_object_take_name (GIMP_OBJECT (palette),
                             g_path_get_basename (gimp_file_get_utf8_name (file)));
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
                       gimp_file_get_utf8_name (file), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            g = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing GREEN component in line %d."),
                       gimp_file_get_utf8_name (file), linenum);

          tok = strtok (NULL, " \t");
          if (tok)
            b = atoi (tok);
          else
            g_message (_("Reading palette file '%s': "
                         "Missing BLUE component in line %d."),
                       gimp_file_get_utf8_name (file), linenum);

          /* optional name */
          tok = strtok (NULL, "\n");

          if (r < 0 || r > 255 ||
              g < 0 || g > 255 ||
              b < 0 || b > 255)
            g_message (_("Reading palette file '%s': "
                         "RGB value out of range in line %d."),
                       gimp_file_get_utf8_name (file), linenum);

          /* don't call gimp_palette_add_entry here, it's rather inefficient */
          entry = g_slice_new0 (GimpPaletteEntry);

          gimp_rgba_set_uchar (&entry->color,
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
                     gimp_file_get_utf8_name (file),
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
gimp_palette_load_act (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  guchar       color_bytes[3];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name = g_path_get_basename (gimp_file_get_utf8_name (file));
  palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
  g_free (palette_name);

  while (g_input_stream_read_all (input, color_bytes, sizeof (color_bytes),
                                  &bytes_read, NULL, NULL) &&
         bytes_read == sizeof (color_bytes))
    {
      GimpRGB color;

      gimp_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_riff (GimpContext   *context,
                        GFile         *file,
                        GInputStream  *input,
                        GError       **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  guchar       color_bytes[4];
  gsize        bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name = g_path_get_basename (gimp_file_get_utf8_name (file));
  palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
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
      GimpRGB color;

      gimp_rgba_set_uchar (&color,
                           color_bytes[0],
                           color_bytes[1],
                           color_bytes[2],
                           255);
      gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_psp (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette *palette;
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

  palette_name = g_path_get_basename (gimp_file_get_utf8_name (file));
  palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
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
                      gimp_file_get_utf8_name (file));
          break;
        }

      ascii_colors = g_strsplit (lines[i + 1], " ", 3);
      color_ok = TRUE;

      for (j = 0 ; j < 3; j++)
        {
          if (ascii_colors[j] == NULL)
            {
              g_printerr ("Corrupted palette file %s.",
                          gimp_file_get_utf8_name (file));
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

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_aco (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette *palette;
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
                      gimp_file_get_utf8_name (file));
      return NULL;
    }

  palette_name = g_path_get_basename (gimp_file_get_utf8_name (file));
  palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
  g_free (palette_name);

  format_version   = header[1] + (header[0] << 8);
  number_of_colors = header[3] + (header[2] << 8);

  for (i = 0; i < number_of_colors; i++)
    {
      gchar       color_info[10];
      gint        color_space;
      gint        w, x, y, z;
      gint        xLab;
      gint        yLab;
      void       *pixels     = NULL;
      const Babl *src_format = NULL;
      const Babl *dst_format = babl_format ("R'G'B' double");
      gboolean    color_ok   = FALSE;
      GimpRGB     color;
      GError     *my_error   = NULL;

      if (! g_input_stream_read_all (input, color_info, sizeof (color_info),
                                     &bytes_read, NULL, &my_error) ||
          bytes_read != sizeof (color_info))
        {
          if (palette->colors)
            {
              g_message (_("Reading palette file '%s': "
                           "Read %d colors from truncated file: %s"),
                         gimp_file_get_utf8_name (file),
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
      /* Lab-specific values */
      xLab = (gint) color_info[5] | ((gint) color_info[4] << 8);
      yLab = (gint) color_info[7] | ((gint) color_info[6] << 8);
      if (xLab >= 32768)
        xLab -= 65536;
      if (yLab >= 32768)
        yLab -= 65536;

      if (color_space == 0) /* RGB */
        {
          gdouble rgb[3] = { ((gdouble) w) / 65536.0,
                             ((gdouble) x) / 65536.0,
                             ((gdouble) y) / 65536.0 };

          src_format = babl_format ("R'G'B' double");
          pixels     = rgb;

          color_ok = TRUE;
        }
      else if (color_space == 1) /* HSV */
        {
          gdouble hsv[3] = { ((gdouble) w) / 65536.0,
                             ((gdouble) x) / 65536.0,
                             ((gdouble) y) / 65536.0};

          src_format = babl_format ("HSV double");
          pixels     = hsv;

          color_ok = TRUE;
        }
      else if (color_space == 2) /* CMYK */
        {
          gdouble cmyk[4] = { 1.0 - ((gdouble) w) / 65536.0,
                              1.0 - ((gdouble) x) / 65536.0,
                              1.0 - ((gdouble) y) / 65536.0,
                              1.0 - ((gdouble) z) / 65536.0 };

          src_format = babl_format ("CMYK double");
          pixels     = cmyk;

          color_ok = TRUE;
        }
      else if (color_space == 7) /* CIE Lab */
        {
          gdouble lab[3] = { ((gdouble) w) / 10000.0,
                             ((gdouble) xLab) / 25500.0,
                             ((gdouble) yLab) / 25500.0 };

          src_format = babl_format ("CIE Lab float");
          pixels     = lab;

          color_ok = TRUE;
        }
      else if (color_space == 8) /* Grayscale */
        {
          gdouble k[1] = { 1.0 - (((gdouble) w) / 10000.0) };

          src_format = babl_format ("Y' double");
          pixels     = k;

          color_ok = TRUE;
        }
      else if (color_space == 9) /* Wide? CMYK */
        {
          gdouble cmyk[4] = { 1.0 - ((gdouble) w) / 10000.0,
                              1.0 - ((gdouble) x) / 10000.0,
                              1.0 - ((gdouble) y) / 10000.0,
                              1.0 - ((gdouble) z) / 10000.0 };

          src_format = babl_format ("CMYK double");
          pixels     = cmyk;

          color_ok = TRUE;
        }
      else
        {
          g_printerr ("Unsupported color space (%d) in ACO file %s\n",
                      color_space, gimp_file_get_utf8_name (file));
        }

      if (color_ok)
        babl_process (babl_fish (src_format, dst_format), (gdouble *) pixels,
                      &color, 1);

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
        gimp_palette_add_entry (palette, -1, NULL, &color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_ase (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette *palette;
  gchar       *palette_name;
  goffset      file_size;
  gint         num_cols;
  gint         i;
  gchar        header[8];
  gshort       group;
  gsize        bytes_read;
  gboolean     skip_first = FALSE;
  const Babl  *src_format = NULL;
  const Babl  *dst_format = babl_format ("R'G'B' double");

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* Get file size */
  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_END, NULL, error);
  file_size = g_seekable_tell (G_SEEKABLE (input));
  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, error);

  if (! g_input_stream_read_all (input, header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      g_prefix_error (error,
                      _("Could not read header from palette file '%s': "),
                      gimp_file_get_utf8_name (file));
      return NULL;
    }


  /* Checking header values */
  if (! g_str_has_prefix (header + 0, "ASEF") ||
      header[5] != 0x01)
    {
      g_prefix_error (error, _("Invalid ASE header: %s"),
                      gimp_file_get_utf8_name (file));
      return NULL;
    }

  if (! g_input_stream_read_all (input, &num_cols, sizeof (num_cols),
                                 &bytes_read, NULL, error))
    {
      g_prefix_error (error,
                      _("Invalid number of colors in palette."));
      return NULL;
    }
  num_cols = GINT32_FROM_BE (num_cols);
  if (num_cols <= 1)
    {
      g_prefix_error (error, _("Invalid number of colors: %s."),
                      gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* First block contains the palette name if
   * one is defined. */
  if (! g_input_stream_read_all (input, &group, sizeof (group),
                                 &bytes_read, NULL, error))
    {
      g_prefix_error (error, _("Invalid ASE file: %s."),
                      gimp_file_get_utf8_name (file));
      return NULL;
    }
  group = GINT16_FROM_BE (group);

  /* If first marker is 0x01, then the palette has no group name */
  if (group != 1)
    {
      palette_name = gimp_palette_load_ase_block_name (input, file_size,
                                                       error);
      palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
      num_cols -= 1;
    }
  else
    {
      palette_name = g_path_get_basename (gimp_file_get_utf8_name (file));
      palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
      skip_first = TRUE;
    }
  g_free (palette_name);

  /* Header blocks are considered a "color" so we offset the count here */
  num_cols -= 1;

  for (i = 0; i < num_cols; i++)
    {
      gchar    color_space[4];
      gchar   *color_name;
      GimpRGB  color;
      gshort   spot_color;
      gfloat  *pixels;
      gint     components  = 0;
      gboolean valid_color = TRUE;

      if (! skip_first)
        {
          if (! g_input_stream_read_all (input, &group, sizeof (group),
                                         &bytes_read, NULL, error))
            {
              g_printerr ("Invalid ASE color entry: %s.",
                          gimp_file_get_utf8_name (file));
              break;
            }
        }
      skip_first = FALSE;

      color_name = gimp_palette_load_ase_block_name (input, file_size, error);
      if (! color_name)
        break;

      g_input_stream_read_all (input, color_space, sizeof (color_space),
                               &bytes_read, NULL, error);

      /* Color formats */
      if (g_str_has_prefix (color_space, "RGB"))
        {
          components = 3;
          src_format = babl_format ("R'G'B' float");
        }
      else if (g_str_has_prefix (color_space, "GRAY"))
        {
          components = 1;
          src_format = babl_format ("Y' float");
        }
      else if (g_str_has_prefix (color_space, "CMYK"))
        {
          components = 4;
          src_format = babl_format ("CMYK float");
        }
      else if (g_str_has_prefix (color_space, "LAB"))
        {
          components = 3;
          src_format = babl_format ("CIE Lab float");
        }

      if (components == 0)
        {
          g_printerr (_("Invalid color components: %s."), color_space);
          g_free (color_name);
          break;
        }

      pixels = g_malloc (sizeof (gfloat) * components);

      for (gint j = 0; j < components; j++)
        {
          gint tmp;

          if (! g_input_stream_read_all (input, &tmp, sizeof (tmp),
                                         &bytes_read, NULL, error))
            {
              g_printerr (_("Invalid ASE color entry: %s."),
                          gimp_file_get_utf8_name (file));
              g_free (color_name);
              valid_color = FALSE;
              break;
            }

          /* Convert 4 bytes to a 32bit float value */
          tmp = GINT32_FROM_BE (tmp);
          pixels[j] = *(gfloat *) &tmp;
        }

      if (! valid_color)
        break;

      /* The L component of LAB goes from 0 to 100 percent */
      if (g_str_has_prefix (color_space, "LAB"))
        pixels[0] *= 100;

      babl_process (babl_fish (src_format, dst_format), pixels, &color, 1);
      g_free (pixels);

      /* TODO: When GIMP supports spot colors, use this information in
       * the palette. */
      if (! g_input_stream_read_all (input, &spot_color, sizeof (spot_color),
                                     &bytes_read, NULL, error))
        {
          g_printerr (_("Invalid ASE color entry: %s."),
                      gimp_file_get_utf8_name (file));
          g_free (color_name);
          break;
        }

      gimp_palette_add_entry (palette, -1, color_name, &color);
      g_free (color_name);
    }

  return g_list_prepend (NULL, palette);
}

static gchar *
gimp_palette_load_ase_block_name (GInputStream  *input,
                                  goffset        file_size,
                                  GError       **error)
{
  gint        block_length;
  gushort     pal_name_len;
  gunichar2  *pal_name = NULL;
  gchar      *pal_name_utf8;
  goffset     current_pos;
  gsize       bytes_read;

  if (! g_input_stream_read_all (input, &block_length, sizeof (block_length),
                                 &bytes_read, NULL, error))
    {
      g_printerr (_("Invalid ASE palette name."));
      return NULL;
    }

  block_length = GINT32_FROM_BE (block_length);
  current_pos  = g_seekable_tell (G_SEEKABLE (input));

  if (block_length <= 0 || block_length > (file_size - current_pos))
    {
      g_printerr (_("Invalid ASE block size."));
      return NULL;
    }

  if (! g_input_stream_read_all (input, &pal_name_len, sizeof (pal_name_len),
                                 &bytes_read, NULL, error))
    {
      g_printerr (_("Invalid ASE palette name."));
      return NULL;
    }

  pal_name_len = GUINT16_FROM_BE (pal_name_len);
  current_pos  = g_seekable_tell (G_SEEKABLE (input));

  if (pal_name_len <= 0 || pal_name_len > (file_size - current_pos))
    {
      g_printerr (_("Invalid ASE name size."));
      return NULL;
    }
  pal_name = g_malloc (pal_name_len * 2);

  for (gint i = 0; i < pal_name_len; i++)
    {
      if (! g_input_stream_read_all (input, &pal_name[i], 2,
                                     &bytes_read, NULL, error))
        {
          g_printerr (_("Invalid ASE palette name."));
          g_free (pal_name);
          return NULL;
        }

      pal_name[i] = GUINT16_FROM_BE (pal_name[i]);
    }

  pal_name_utf8 = g_utf16_to_utf8 (pal_name, -1, NULL, NULL, NULL);
  g_free (pal_name);

  return pal_name_utf8;
}

GList *
gimp_palette_load_css (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette      *palette;
  GDataInputStream *data_input;
  gchar            *name;
  GRegex           *regex;
  gchar            *buf;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  regex = g_regex_new (".*color.*:(?P<param>.*)", G_REGEX_CASELESS, 0, error);
  if (! regex)
    return NULL;

  name = g_path_get_basename (gimp_file_get_utf8_name (file));
  palette = GIMP_PALETTE (gimp_palette_new (context, name));
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
              GimpRGB  color;
              gchar   *word = g_match_info_fetch_named (matches, "param");

              if (gimp_rgb_parse_css (&color, word, -1))
                {
                  if (! gimp_palette_find_entry (palette, &color, NULL))
                    {
                      gimp_palette_add_entry (palette, -1, NULL, &color);
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

GList *
gimp_palette_load_sbz (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  SwatchBookerData      sbz_data;
  gchar                *palette_name;
  gchar                *xml_data = NULL;
  struct archive       *a;
  struct archive_entry *entry;
  size_t                entry_size;
  int                   r;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  palette_name     = g_path_get_basename (gimp_file_get_utf8_name (file));
  sbz_data.palette = GIMP_PALETTE (gimp_palette_new (context, palette_name));
  g_free (palette_name);

  sbz_data.position          = 0;
  sbz_data.sorted_position   = 0;
  sbz_data.in_color_tag      = FALSE;
  sbz_data.in_book_tag       = FALSE;
  sbz_data.copy_name         = FALSE;
  sbz_data.copy_values       = FALSE;
  sbz_data.embedded_profiles = NULL;

  if ((a = archive_read_new ()))
    {
      const gchar *name = gimp_file_get_utf8_name (file);

      archive_read_support_format_all (a);
      r = archive_read_open_filename (a, name, 10240);
      if (r != ARCHIVE_OK)
        {
          archive_read_free (a);

          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Unable to read SBZ file"));
          return NULL;
        }

      while (archive_read_next_header (a, &entry) == ARCHIVE_OK)
        {
          const gchar *lower = g_ascii_strdown (archive_entry_pathname (entry), -1);

          if (g_str_has_suffix (lower, ".xml"))
            {
              entry_size = archive_entry_size (entry);
              xml_data   = (gchar *) g_malloc (entry_size);

              r = archive_read_data (a, xml_data, entry_size);
            }
          else if (g_str_has_suffix (lower, ".icc") || g_str_has_suffix (lower, ".icm"))
            {
              GimpColorProfile *profile  = NULL;
              size_t            icc_size = archive_entry_size (entry);
              uint8_t          *icc_data = g_malloc (icc_size);

              r = archive_read_data (a, icc_data, icc_size);

              if (icc_data)
                profile = gimp_color_profile_new_from_icc_profile (icc_data, icc_size,
                                                                   NULL);

              if (profile)
                {
                  SwatchBookerColorProfile sbz_profile;

                  sbz_profile.profile = profile;
                  sbz_profile.id = g_strdup (archive_entry_pathname (entry));

                  sbz_data.embedded_profiles =
                    g_list_append (sbz_data.embedded_profiles, &sbz_profile);
                }
            }
        }

      if (xml_data)
        {
          GimpXmlParser *xml_parser;
          GMarkupParser  markup_parser;

          markup_parser.start_element = swatchbooker_load_start_element;
          markup_parser.end_element   = swatchbooker_load_end_element;
          markup_parser.text          = swatchbooker_load_text;
          markup_parser.passthrough   = NULL;
          markup_parser.error         = NULL;

          xml_parser = gimp_xml_parser_new (&markup_parser, &sbz_data);

          gimp_xml_parser_parse_buffer (xml_parser, xml_data, entry_size, NULL);
          gimp_xml_parser_free (xml_parser);

          g_free (xml_data);
        }

      r = archive_read_free (a);
    }
  else
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unable to open SBZ file"));
      return NULL;
    }

  return g_list_prepend (NULL, sbz_data.palette);
}

static void
swatchbooker_load_start_element (GMarkupParseContext *context,
                                 const gchar         *element_name,
                                 const gchar        **attribute_names,
                                 const gchar        **attribute_values,
                                 gpointer             user_data,
                                 GError             **error)
{
  SwatchBookerData *sbz_data = user_data;

  sbz_data->copy_values = FALSE;
  sbz_data->color_model = NULL;
  sbz_data->color_space = NULL;

  if (! strcmp (g_ascii_strdown (element_name, -1), "color"))
    {
      sbz_data->in_color_tag = TRUE;
    }
  else if (! strcmp (g_ascii_strdown (element_name, -1), "dc:identifier"))
    {
      if (sbz_data->in_color_tag)
        sbz_data->copy_name = TRUE;
    }
  else if (! strcmp (g_ascii_strdown (element_name, -1), "values"))
    {
      while (*attribute_names)
        {
          if (! strcmp (g_ascii_strdown (*attribute_names, -1), "model"))
            {
              sbz_data->color_model =
                g_strdup (g_ascii_strdown (*attribute_values, -1));
            }
          else if (! strcmp (g_ascii_strdown (*attribute_names, -1), "space"))
            {
              sbz_data->color_space = g_strdup (*attribute_values);
            }

          attribute_names++;
          attribute_values++;
        }

      sbz_data->copy_values = TRUE;
    }
  else if (! strcmp (g_ascii_strdown (element_name, -1), "book"))
    {
      sbz_data->in_book_tag = TRUE;

      while (*attribute_names)
        {
          if (! strcmp (g_ascii_strdown (*attribute_names, -1), "columns"))
            {
              gint columns = atoi (*attribute_values);

              if (columns > 0)
                gimp_palette_set_columns (sbz_data->palette, columns);

              break;
            }

          attribute_names++;
          attribute_values++;
        }
    }
  else if (! strcmp (g_ascii_strdown (element_name, -1), "swatch") &&
           sbz_data->in_book_tag)
    {
      while (*attribute_names)
        {
          if (! strcmp (g_ascii_strdown (*attribute_names, -1), "material"))
            {
              GList *cols;
              gint   original_id = 0;

              for (cols = gimp_palette_get_colors (sbz_data->palette);
                   cols;
                   cols = g_list_next (cols))
                {
                  GimpPaletteEntry *entry = cols->data;

                  if (! strcmp (*attribute_values, entry->name))
                    {
                      gimp_palette_move_entry (sbz_data->palette, entry,
                                               sbz_data->sorted_position);

                      sbz_data->sorted_position++;
                      break;
                    }
                  original_id++;
                }
              break;
            }

          attribute_names++;
          attribute_values++;
        }
    }
}

static void
swatchbooker_load_end_element (GMarkupParseContext *context,
                               const gchar         *element_name,
                               gpointer             user_data,
                               GError             **error)
{
  SwatchBookerData *sbz_data = user_data;

  if (! strcmp (g_ascii_strdown (element_name, -1), "color"))
    sbz_data->in_color_tag = FALSE;
  else if (! strcmp (g_ascii_strdown (element_name, -1), "book"))
    sbz_data->in_book_tag = FALSE;
}

static void
swatchbooker_load_text (GMarkupParseContext *context,
                        const gchar         *text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error)
{
  SwatchBookerData  *sbz_data = user_data;
  gchar            **values;
  gint               i;
  gint               total = 0;

  /* Save palette name */
  if (sbz_data->copy_name)
    {
      if (! sbz_data->palette_name)
        sbz_data->palette_name = g_strdup (text);

      sbz_data->copy_name = FALSE;
    }

  /* Load palette entry */
  if (sbz_data->copy_values)
    {
      values = g_strsplit (text, " ", 0);

      for (i = 0; values[i]; i++)
        total++;

      if (total > 0)
        {
          gfloat      true_values[total];
          GimpRGB     color;
          gboolean    load_pal   = FALSE;
          const Babl *src_format = NULL;
          const Babl *dst_format = babl_format ("R'G'B' float");
          const Babl *space      = NULL;

          /* Load profile if applicable */
          if (sbz_data->embedded_profiles &&
              sbz_data->color_space)
            {
              GList *profile_list;

              for (profile_list = g_list_copy (sbz_data->embedded_profiles);
                   profile_list;
                   profile_list = g_list_next (profile_list))
                {
                  SwatchBookerColorProfile *icc = profile_list->data;

                  if (! strcmp (sbz_data->color_space, icc->id))
                    {
                      space = gimp_color_profile_get_space (icc->profile,
                                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                            NULL);
                      break;
                    }
                }
            }

          for (i = 0; values[i]; i++)
            true_values[i] = atof (values[i]);

          /* No need for babl conversion for sRGB colors */
          if (! strcmp (sbz_data->color_model, "srgb"))
            {
              gimp_rgb_set (&color, true_values[0], true_values[1],
                            true_values[2]);
              load_pal = TRUE;
            }
          else if (! strcmp (sbz_data->color_model, "rgb"))
            {
              src_format = babl_format_with_space ("R'G'B' float", space);
            }
          else if (! strcmp (sbz_data->color_model, "gray"))
            {
              src_format = babl_format_with_space ("Y' float", space);
            }
          else if (! strcmp (sbz_data->color_model, "cmyk"))
            {
              src_format = babl_format_with_space ("CMYK float", space);
            }
          else if (! strcmp (sbz_data->color_model, "hsl"))
            {
              src_format = babl_format_with_space ("HSL float", space);
            }
          else if (! strcmp (sbz_data->color_model, "hsv"))
            {
              src_format = babl_format_with_space ("HSV float", space);
            }
          else if (! strcmp (sbz_data->color_model, "lab"))
            {
              src_format = babl_format_with_space ("CIE Lab float", space);
            }
          else if (! strcmp (sbz_data->color_model, "xyz"))
            {
              src_format = babl_format_with_space ("CIE XYZ float", space);
            }

          if (src_format != NULL)
            {
              gfloat rgb[3];

              babl_process (babl_fish (src_format, dst_format),
                            true_values, rgb, 1);
              gimp_rgb_set (&color, rgb[0], rgb[1], rgb[2]);
              load_pal = TRUE;
            }

          if (load_pal)
            {
              gimp_palette_add_entry (sbz_data->palette, sbz_data->position,
                                      NULL, &color);
              if (sbz_data->palette_name)
                gimp_palette_set_entry_name (sbz_data->palette,
                                             sbz_data->position,
                                             sbz_data->palette_name);
              sbz_data->position++;
            }
        }

      sbz_data->palette_name = NULL;
      sbz_data->copy_values  = FALSE;
      if (sbz_data->color_model)
        g_free (sbz_data->color_model);
    }
}

GimpPaletteFileFormat
gimp_palette_load_detect_format (GFile        *file,
                                 GInputStream *input)
{
  GimpPaletteFileFormat format = GIMP_PALETTE_FILE_FORMAT_UNKNOWN;
  gchar                 header[16];
  gsize                 bytes_read;

  if (g_input_stream_read_all (input, &header, sizeof (header),
                               &bytes_read, NULL, NULL) &&
      bytes_read == sizeof (header))
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

  if (format == GIMP_PALETTE_FILE_FORMAT_UNKNOWN)
    {
      gchar *lower = g_ascii_strdown (gimp_file_get_utf8_name (file), -1);

      if (g_str_has_suffix (lower, ".aco"))
        {
          format = GIMP_PALETTE_FILE_FORMAT_ACO;
        }
      if (g_str_has_suffix (lower, ".ase"))
        {
          format = GIMP_PALETTE_FILE_FORMAT_ASE;
        }
      else if (g_str_has_suffix (lower, ".css"))
        {
          format = GIMP_PALETTE_FILE_FORMAT_CSS;
        }
      else if (g_str_has_suffix (lower, ".sbz"))
        {
          format = GIMP_PALETTE_FILE_FORMAT_SBZ;
        }

      g_free (lower);
    }

  if (format == GIMP_PALETTE_FILE_FORMAT_UNKNOWN)
    {
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                           G_FILE_QUERY_INFO_NONE,
                                           NULL, NULL);

      if (info)
        {
          goffset size = g_file_info_get_size (info);

          if (size == 768)
            format = GIMP_PALETTE_FILE_FORMAT_ACT;

          g_object_unref (info);
        }
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  return format;
}
