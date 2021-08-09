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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppattern.h"
#include "gimppattern-header.h"
#include "gimppattern-load.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


GList *
gimp_pattern_load (GimpContext   *context,
                   GFile         *file,
                   GInputStream  *input,
                   GError       **error)
{
  GimpPattern       *pattern = NULL;
  const Babl        *format  = NULL;
  GimpPatternHeader  header;
  gsize              size;
  gsize              bytes_read;
  gsize              bn_size;
  gchar             *name = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /*  read the size  */
  if (! g_input_stream_read_all (input, &header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      g_prefix_error (error, _("File appears truncated: "));
      goto error;
    }

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);

  /*  Check for correct file format */
  if (header.magic_number != GIMP_PATTERN_MAGIC ||
      header.version      != 1                  ||
      header.header_size  <= sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unknown pattern format version %d."),
                   header.version);
      goto error;
    }

  /*  Check for supported bit depths  */
  if (header.bytes < 1 || header.bytes > 4)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unsupported pattern depth %d.\n"
                     "GIMP Patterns must be GRAY or RGB."),
                   header.bytes);
      goto error;
    }

  /*  Validate dimensions  */
  if ((header.width  == 0) || (header.width  > GIMP_PATTERN_MAX_SIZE) ||
      (header.height == 0) || (header.height > GIMP_PATTERN_MAX_SIZE) ||
      (G_MAXSIZE / header.width / header.height / header.bytes < 1))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid header data in '%s': width=%lu (maximum %lu), "
                     "height=%lu (maximum %lu), bytes=%lu"),
                   gimp_file_get_utf8_name (file),
                   (gulong) header.width,  (gulong) GIMP_PATTERN_MAX_SIZE,
                   (gulong) header.height, (gulong) GIMP_PATTERN_MAX_SIZE,
                   (gulong) header.bytes);
      goto error;
    }

  /*  Read in the pattern name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      if (bn_size > GIMP_PATTERN_MAX_NAME)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Invalid header data in '%s': "
                         "Pattern name is too long: %lu"),
                       gimp_file_get_utf8_name (file),
                       (gulong) bn_size);
          goto error;
        }

      name = g_new0 (gchar, bn_size + 1);

      if (! g_input_stream_read_all (input, name, bn_size,
                                     &bytes_read, NULL, error) ||
          bytes_read != bn_size)
        {
          g_prefix_error (error, _("File appears truncated."));
          g_free (name);
          goto error;
        }

      utf8 = gimp_any_to_utf8 (name, bn_size - 1,
                               _("Invalid UTF-8 string in pattern file '%s'."),
                               gimp_file_get_utf8_name (file));
      g_free (name);
      name = utf8;
    }

  if (! name)
    name = g_strdup (_("Unnamed"));

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-gimp-pat",
                          NULL);

  g_free (name);

  switch (header.bytes)
    {
    case 1: format = babl_format ("Y' u8");      break;
    case 2: format = babl_format ("Y'A u8");     break;
    case 3: format = babl_format ("R'G'B' u8");  break;
    case 4: format = babl_format ("R'G'B'A u8"); break;
    }

  pattern->mask = gimp_temp_buf_new (header.width, header.height, format);
  size = (gsize) header.width * header.height * header.bytes;

  if (! g_input_stream_read_all (input,
                                 gimp_temp_buf_get_data (pattern->mask), size,
                                 &bytes_read, NULL, error) ||
      bytes_read != size)
    {
      g_prefix_error (error, _("File appears truncated."));
      goto error;
    }

  return g_list_prepend (NULL, pattern);

 error:

  if (pattern)
    g_object_unref (pattern);

  g_prefix_error (error, _("Fatal parse error in pattern file: "));

  return NULL;
}

GList *
gimp_pattern_load_pixbuf (GimpContext   *context,
                          GFile         *file,
                          GInputStream  *input,
                          GError       **error)
{
  GimpPattern *pattern;
  GdkPixbuf   *pixbuf;
  gchar       *name;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pixbuf = gdk_pixbuf_new_from_stream (input, NULL, error);
  if (! pixbuf)
    return NULL;

  name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Title"));

  if (! name)
    name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Comment"));

  if (! name)
    name = g_path_get_basename (gimp_file_get_utf8_name (file));

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", NULL, /* FIXME!! */
                          NULL);
  g_free (name);

  pattern->mask = gimp_temp_buf_new_from_pixbuf (pixbuf, NULL);

  g_object_unref (pixbuf);

  return g_list_prepend (NULL, pattern);
}
