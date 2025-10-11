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

static gchar * gimp_palette_load_acb_string     (GInputStream  *input,
                                                 goffset        file_size,
                                                 GError       **error);
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
  GDataInputStream *data_input;
  gchar            *str;
  gsize             str_len;
  gchar            *tok;
  gint              linenum;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data_input = g_data_input_stream_new (input);

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
          GeglColor *color  = gegl_color_new ("black");
          guint8     rgb[3] = { 0 };

          tok = strtok (str, " \t");
          if (tok)
            {
              if (atoi (tok) < 0 || atoi (tok) > 255)
                g_message (_("Reading palette file '%s': "
                             "red component out of range in line %d."),
                           gimp_file_get_utf8_name (file), linenum);

              rgb[0] = CLAMP (atoi (tok), 0, 255);
            }
          else
            {
              g_message (_("Reading palette file '%s': "
                           "Missing RED component in line %d."),
                         gimp_file_get_utf8_name (file), linenum);
            }

          tok = strtok (NULL, " \t");
          if (tok)
            {
              if (atoi (tok) < 0 || atoi (tok) > 255)
                g_message (_("Reading palette file '%s': "
                             "green component out of range in line %d."),
                           gimp_file_get_utf8_name (file), linenum);

              rgb[1] = CLAMP (atoi (tok), 0, 255);
            }
          else
            {
              g_message (_("Reading palette file '%s': "
                           "Missing GREEN component in line %d."),
                         gimp_file_get_utf8_name (file), linenum);
            }

          tok = strtok (NULL, " \t");
          if (tok)
            {
              if (atoi (tok) < 0 || atoi (tok) > 255)
                g_message (_("Reading palette file '%s': "
                             "blue component out of range in line %d."),
                           gimp_file_get_utf8_name (file), linenum);

              rgb[2] = CLAMP (atoi (tok), 0, 255);
            }
          else
            {
              g_message (_("Reading palette file '%s': "
                           "Missing BLUE component in line %d."),
                         gimp_file_get_utf8_name (file), linenum);
            }

          /* optional name */
          tok = strtok (NULL, "\n");

          /* Historical .gpl format is sRGB. */
          gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), rgb);
          gimp_palette_add_entry (palette, -1, tok, color);
          g_object_unref (color);
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
      /* The spec is absolutely unclear and just says:
       * > If loaded into the Colors palette, the colors will be installed in the color swatch list as RGB colors.
       * So should these be sRGB colors? Anyway since palettes are not attached
       * to images (except in specific case of indexed images), we can't use a
       * specific image's color space and the format simply has no color space
       * information.
       * So we just assume these colors are sRGB.
       */
      GeglColor *color = gegl_color_new (NULL);

      gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), color_bytes);
      gimp_palette_add_entry (palette, -1, NULL, color);

      g_object_unref (color);
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
      GeglColor *color = gegl_color_new (NULL);

      /* The format does not include color space information. The fourth byte is
       * called peFlags but possible values in Microsoft documentation does not
       * list space-related flags.
       * So as often with such historical formats, we assume that by "RGB" they
       * meant "sRGB".
       */
      gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), color_bytes);
      gimp_palette_add_entry (palette, -1, NULL, color);

      g_object_unref (color);
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
          GeglColor *color = gegl_color_new (NULL);

          /* The format does not include color space information. Assuming sRGB
           * like for all such historical formats.
           */
          gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), color_bytes);
          gimp_palette_add_entry (palette, -1, NULL, color);

          g_object_unref (color);
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
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
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
      GeglColor  *color;
      gchar       color_info[10];
      gint        color_space;
      gint        w, x, y, z;
      gint        xLab;
      gint        yLab;
      gboolean    color_ok   = FALSE;
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

      color = gegl_color_new (NULL);
      if (color_space == 0) /* RGB */
        {
          gdouble rgb[3] = { ((gdouble) w) / 65536.0,
                             ((gdouble) x) / 65536.0,
                             ((gdouble) y) / 65536.0 };

          /* No color space information in the format. */
          gegl_color_set_pixel (color, babl_format ("R'G'B' double"), rgb);
          color_ok = TRUE;
        }
      else if (color_space == 1) /* HSV */
        {
          gfloat hsv[3] = { ((gfloat) w) / 65536.0f,
                            ((gfloat) x) / 65536.0f,
                            ((gfloat) y) / 65536.0f};

          gegl_color_set_pixel (color, babl_format ("HSV float"), hsv);
          color_ok = TRUE;
        }
      else if (color_space == 2) /* CMYK */
        {
          float cmyk[4] = { 1.0f - ((gfloat) w) / 65536.0f,
                            1.0f - ((gfloat) x) / 65536.0f,
                            1.0f - ((gfloat) y) / 65536.0f,
                            1.0f - ((gfloat) z) / 65536.0f };

          /* No color space information in the format. */
          gegl_color_set_pixel (color, babl_format ("CMYK float"), cmyk);
          color_ok = TRUE;
        }
      else if (color_space == 7) /* CIE Lab */
        {
          gfloat lab[3] = { ((gfloat) w) / 10000.0f,
                            ((gfloat) xLab) / 25500.0f,
                            ((gfloat) yLab) / 25500.0f };

          gegl_color_set_pixel (color, babl_format ("CIE Lab float"), lab);
          color_ok = TRUE;
        }
      else if (color_space == 8) /* Grayscale */
        {
          gdouble k[1] = { 1.0 - (((gdouble) w) / 10000.0) };

          /* No color space information in the format. */
          gegl_color_set_pixel (color, babl_format ("Y' double"), k);
          color_ok = TRUE;
        }
      else if (color_space == 9) /* Wide? CMYK */
        {
          /* XXX: this "Color Space ID" 8 does not exist in Adobe specification.
           * Apparently this case was present ever since the first
           * implementation in GIMP so it's hard to know the origin behind it.
           * Where does it come from?
           */
          gfloat cmyk[4] = { 1.0f - ((gfloat) w) / 10000.0,
                             1.0f - ((gfloat) x) / 10000.0,
                             1.0f - ((gfloat) y) / 10000.0,
                             1.0f - ((gfloat) z) / 10000.0 };

          gegl_color_set_pixel (color, babl_format ("CMYK float"), cmyk);
          color_ok = TRUE;
        }
      else
        {
          g_printerr ("Unsupported color space (%d) in ACO file %s\n",
                      color_space, gimp_file_get_utf8_name (file));
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
              g_object_unref (color);
              return NULL;
            }

          number_of_chars = format2_preamble[3] + (format2_preamble[2] << 8);

          /* TODO: color name should be used (by converting from UTF-16 to
           * UTF-8), not ignored. Also according to spec, there should be 2
           * additional NUL bytes at the end, which are not (I think) counted in
           * number_of_chars). Therefore this code may be wrong and missing 2
           * bytes.
           */
          if (! g_seekable_seek (G_SEEKABLE (input), number_of_chars * 2,
                                 G_SEEK_SET, NULL, error))
            {
              g_object_unref (palette);
              g_object_unref (color);
              return NULL;
            }
        }

      if (color_ok)
        gimp_palette_add_entry (palette, -1, NULL, color);

      g_object_unref (color);
    }

  return g_list_prepend (NULL, palette);
}

GList *
gimp_palette_load_acb (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpPalette  *palette;
  gchar         header[4];
  gshort        version;
  gchar       **palette_strings;
  gchar        *palette_name;
  gchar       **palette_prefix;
  gchar       **palette_suffix;
  gchar        *description;
  gushort       number_of_colors;
  gushort       page;
  gushort       color_space;
  gint          i;
  goffset       file_size;
  gsize         bytes_read;

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
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Could not read header from palette file '%s': "),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* Version and ID */
  if (! g_input_stream_read_all (input, &version, sizeof (version),
                                 &bytes_read, NULL, error))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette version."));
      return NULL;
    }
  version = GUINT16_FROM_BE (version);
  if (version != 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("GIMP only supports version 1 ACB palettes"));
      return NULL;
    }

  if (! g_input_stream_read_all (input, &version, sizeof (version),
                                 &bytes_read, NULL, error))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette identifier."));
      return NULL;
    }

  /* Color Book name and palette prefix/suffix */
  palette_name = gimp_palette_load_acb_string (input, file_size, error);
  if (palette_name)
    palette_strings = g_strsplit (palette_name, "=", -1);
  if (! palette_name || g_strv_length (palette_strings) != 2)
    {
      if (palette_name)
        {
          g_strfreev (palette_strings);
          g_free (palette_name);
        }
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette name."));
      return NULL;
    }
  palette = GIMP_PALETTE (gimp_palette_new (context, palette_strings[1]));

  g_strfreev (palette_strings);
  g_free (palette_name);

  palette_name = gimp_palette_load_acb_string (input, file_size, error);
  if (palette_name)
    palette_prefix = g_strsplit (palette_name, "=", -1);
  if (! palette_name || g_strv_length (palette_prefix) != 2)
    {
      if (palette_name)
        {
          g_strfreev (palette_prefix);
          g_free (palette_name);
        }
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette name."));
      return NULL;
    }
  g_free (palette_name);

  palette_name = gimp_palette_load_acb_string (input, file_size, error);
  if (palette_name)
    palette_suffix = g_strsplit (palette_name, "=", -1);
  if (! palette_name || g_strv_length (palette_suffix) != 2)
    {
      if (palette_name)
        {
          g_strfreev (palette_suffix);
          g_free (palette_name);
        }
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette suffix."));
      return NULL;
    }
  g_free (palette_name);

  /* Description (currently unused) */
  description = gimp_palette_load_acb_string (input, file_size,
                                              error);
  g_free (description);

  /* Number of colors */
  if (! g_input_stream_read_all (input, &number_of_colors,
                                 sizeof (number_of_colors), &bytes_read, NULL,
                                 error))
    {
      g_strfreev (palette_prefix);
      g_strfreev (palette_suffix);

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid number of colors in palette."));
      return NULL;
    }
  number_of_colors = GUINT16_FROM_BE (number_of_colors);
  if (number_of_colors <= 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid number of colors: %s."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* Page information (currently unused) */
  if (! g_input_stream_read_all (input, &page, sizeof (page),
                                 &bytes_read, NULL, error))
    {
      g_strfreev (palette_prefix);
      g_strfreev (palette_suffix);

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette page info."));
      return NULL;
    }
  if (! g_input_stream_read_all (input, &page, sizeof (page),
                                 &bytes_read, NULL, error))
    {
      g_strfreev (palette_prefix);
      g_strfreev (palette_suffix);

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette page info."));
      return NULL;
    }

  /* Color Model */
  if (! g_input_stream_read_all (input, &color_space, sizeof (color_space),
                                 &bytes_read, NULL, error))
    {
      g_strfreev (palette_prefix);
      g_strfreev (palette_suffix);

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ACB palette color space."));
      return NULL;
    }
  color_space = GUINT16_FROM_BE (color_space);

  if (color_space != 0 &&
      color_space != 2 &&
      color_space != 7)
    {
      g_strfreev (palette_prefix);
      g_strfreev (palette_suffix);

     g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                  _("Invalid ACB palette color space."));
      return NULL;
    }

  /* Color records */
  for (i = 0; i < number_of_colors; i++)
    {
      GeglColor *color             = gegl_color_new (NULL);
      gchar      color_code[6];
      gchar     *palette_entry     = NULL;
      gchar     *full_palette_name = NULL;
      gboolean   color_ok          = FALSE;

      palette_entry = gimp_palette_load_acb_string (input, file_size,
                                                    error);

      if (palette_entry)
        full_palette_name = g_strdup_printf ("%s%s %s", palette_prefix[1],
                                             palette_entry, palette_suffix[1]);

      /* Color Code (not used) */
      if (! g_input_stream_read_all (input, color_code, sizeof (color_code),
                                     &bytes_read, NULL, error))
        {
          g_free (palette_entry);
          g_free (full_palette_name);
          g_clear_object (&color);

          g_printerr ("Invalid ACB palette color code");
          break;
        }

      if (color_space == 0)
        {
          guchar rgb[3];

          if (! g_input_stream_read_all (input, rgb, sizeof (rgb),
                                         &bytes_read, NULL, error))
            {
              g_free (palette_entry);
              g_free (full_palette_name);
              g_clear_object (&color);

              g_printerr ("Invalid ACB palette colors");
              break;
            }

          gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), rgb);
          color_ok = TRUE;
        }
      else if (color_space == 2)
        {
          guchar cmyk[4];
          gfloat cmyk_f[4];

          if (! g_input_stream_read_all (input, cmyk, sizeof (cmyk),
                                         &bytes_read, NULL, error))
            {
              g_free (palette_entry);
              g_free (full_palette_name);
              g_clear_object (&color);

              g_printerr ("Invalid ACB palette colors");
              break;
            }

          for (gint j = 0; j < 4; j++)
            cmyk_f[j] = (((255 - cmyk[j]) / 2.55f) + 0.5f) / 100.0f;

          gegl_color_set_pixel (color, babl_format ("CMYK float"), cmyk_f);
          color_ok = TRUE;
        }
      else if (color_space == 7)
        {
          guchar lab[3];
          gfloat lab_f[3];

          if (! g_input_stream_read_all (input, lab, sizeof (lab),
                                         &bytes_read, NULL, error))
            {
              g_free (palette_entry);
              g_free (full_palette_name);
              g_clear_object (&color);

              g_printerr ("Invalid ACB palette colors");
              break;
            }

          lab_f[0] = (lab[0] / 2.55f) + 0.5f;
          lab_f[1] = ((gfloat) lab[1]) - 128;
          lab_f[2] = ((gfloat) lab[2]) - 128;

          gegl_color_set_pixel (color, babl_format ("CIE Lab float"), lab_f);
          color_ok = TRUE;
        }

      if (color_ok && palette_entry)
        gimp_palette_add_entry (palette, -1, full_palette_name, color);

      g_free (palette_entry);
      g_free (full_palette_name);
      g_clear_object (&color);

      if (! color_ok)
        {
          g_printerr ("Invalid ACB palette colors");
          break;
        }
    }

  g_strfreev (palette_prefix);
  g_strfreev (palette_suffix);

  return g_list_prepend (NULL, palette);
}

static gchar *
gimp_palette_load_acb_string (GInputStream  *input,
                              goffset        file_size,
                              GError       **error)
{
  gint        pal_name_len;
  gunichar2  *pal_name = NULL;
  gchar      *pal_name_utf8;
  goffset     current_pos;
  gsize       bytes_read;

  if (! g_input_stream_read_all (input, &pal_name_len, sizeof (pal_name_len),
                                 &bytes_read, NULL, error))
    {
      g_printerr (_("Invalid ACB palette name."));
      return NULL;
    }

  pal_name_len = GUINT32_FROM_BE (pal_name_len);
  current_pos  = g_seekable_tell (G_SEEKABLE (input));

  if (pal_name_len <= 0 || pal_name_len > (file_size - current_pos))
    {
      if (pal_name_len != 0)
        g_printerr (_("Invalid ACB name size."));

      return NULL;
    }
  pal_name = g_malloc (pal_name_len * 2);

  for (gint i = 0; i < pal_name_len; i++)
    {
      if (! g_input_stream_read_all (input, &pal_name[i], 2,
                                     &bytes_read, NULL, error))
        {
          g_printerr (_("Invalid ACB palette name."));
          g_free (pal_name);
          return NULL;
        }

      pal_name[i] = GUINT16_FROM_BE (pal_name[i]);
    }

  pal_name_utf8 = g_utf16_to_utf8 (pal_name, pal_name_len, NULL, NULL, NULL);
  g_free (pal_name);

  return pal_name_utf8;
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
  gfloat       pixels[4];

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
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Could not read header from palette file '%s': "),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }


  /* Checking header values */
  if (! g_str_has_prefix (header + 0, "ASEF") ||
      header[5] != 0x01)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ASE header: %s"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  if (! g_input_stream_read_all (input, &num_cols, sizeof (num_cols),
                                 &bytes_read, NULL, error))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid number of colors in palette."));
      return NULL;
    }
  num_cols = GINT32_FROM_BE (num_cols);
  if (num_cols <= 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid number of colors: %s."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* First block contains the palette name if
   * one is defined. */
  if (! g_input_stream_read_all (input, &group, sizeof (group),
                                 &bytes_read, NULL, error))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid ASE file: %s."),
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

  for (i = 0; i < num_cols; i++)
    {
      GeglColor *color;
      gchar      color_space[4];
      gchar     *color_name;
      gshort     spot_color;
      gint       components  = 0;
      gboolean   valid_color = TRUE;

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

      /* Skip group marker padding */
      group = GINT16_FROM_BE (group);
      if (group < 0)
        {
          gchar marker[4];

          if (! g_input_stream_read_all (input, &marker, sizeof (marker),
                                         &bytes_read, NULL, error))
            {
              g_printerr ("Invalid ASE group marker: %s.",
                          gimp_file_get_utf8_name (file));
              break;
            }
          num_cols--;
          continue;
        }

      color_name = gimp_palette_load_ase_block_name (input, file_size, error);
      if (! color_name)
        break;

      g_input_stream_read_all (input, color_space, sizeof (color_space),
                               &bytes_read, NULL, error);

      /* Color formats */
      if (g_str_has_prefix (g_strstrip (color_space), "RGB"))
        {
          components = 3;
          src_format = babl_format ("R'G'B' float");
        }
      else if (g_str_has_prefix (g_strstrip (color_space), "GRAY"))
        {
          components = 1;
          src_format = babl_format ("Y' float");
        }
      else if (g_str_has_prefix (g_strstrip (color_space), "CMYK"))
        {
          components = 4;
          src_format = babl_format ("CMYK float");
        }
      else if (g_str_has_prefix (g_strstrip (color_space), "LAB"))
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

      for (gint j = 0; j < components; j++)
        {
          gint32 tmp;

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
          memcpy (&pixels[j], &tmp, 4);
        }

      if (! valid_color)
        break;

      /* The L component of LAB goes from 0 to 100 percent */
      if (g_str_has_prefix (g_strstrip (color_space), "LAB"))
        pixels[0] *= 100;


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

      color = gegl_color_new (NULL);
      gegl_color_set_pixel (color, src_format, pixels);

      gimp_palette_add_entry (palette, -1, color_name, color);
      g_free (color_name);
      g_object_unref (color);
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

  pal_name_utf8 = g_utf16_to_utf8 (pal_name, pal_name_len, NULL, NULL, NULL);
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
              GeglColor *color;
              gchar     *word = g_match_info_fetch_named (matches, "param");

              color = gimp_color_parse_css (word);
              if (color)
                {
                  if (! gimp_palette_find_entry (palette, color, NULL))
                    gimp_palette_add_entry (palette, -1, NULL, color);

                  g_object_unref (color);
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
          GeglColor  *color      = NULL;
          const Babl *src_format = NULL;
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
              src_format = babl_format_with_space ("R'G'B' float", NULL);
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
              color = gegl_color_new (NULL);
              gegl_color_set_pixel (color, src_format, true_values);
            }

          if (color)
            {
              gimp_palette_add_entry (sbz_data->palette, sbz_data->position,
                                      NULL, color);
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
          /* TODO: only the Simple PAL format is supported right now.
           * Extended PAL Format with "PAL plth" header support could be added
           * later on.
           */
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
      else if (g_str_has_prefix (header, "8BCB"))
        {
          format = GIMP_PALETTE_FILE_FORMAT_ACB;
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
          goffset size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);

          /* TODO: the file can optionally be 772 bytes long.
           * See: https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/
           * Section: "Color Table "
           */
          if (size == 768)
            format = GIMP_PALETTE_FILE_FORMAT_ACT;

          g_object_unref (info);
        }
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  return format;
}
