/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp_brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpbrushgenerated.h"
#include "gimpbrushgenerated-load.h"

#include "gimp-intl.h"


GList *
gimp_brush_generated_load (GimpContext  *context,
                           GFile        *file,
                           GError      **error)
{
  GimpBrush               *brush;
  GInputStream            *input;
  GDataInputStream        *data_input;
  gchar                   *string;
  gsize                    string_len;
  gint                     linenum;
  gchar                   *name       = NULL;
  GimpBrushGeneratedShape  shape      = GIMP_BRUSH_GENERATED_CIRCLE;
  gboolean                 have_shape = FALSE;
  gint                     spikes     = 2;
  gdouble                  spacing;
  gdouble                  radius;
  gdouble                  hardness;
  gdouble                  aspect_ratio;
  gdouble                  angle;
  GError                  *my_error = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));
  if (! input)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), my_error->message);
      g_clear_error (&my_error);
      return NULL;
    }
  else
    {
      data_input = g_data_input_stream_new (input);
      g_object_unref (input);
    }

  /* make sure the file we are reading is the right type */
  linenum = 1;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;

  if (! g_str_has_prefix (string, "GIMP-VBR"))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Not a GIMP brush file."),
                   gimp_file_get_utf8_name (file));
      g_free (string);
      goto failed;
    }

  g_free (string);

  /* make sure we are reading a compatible version */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;

  if (! g_str_has_prefix (string, "1.0"))
    {
      if (! g_str_has_prefix (string, "1.5"))
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "Unknown GIMP brush version in line %d."),
                       gimp_file_get_utf8_name (file), linenum);
          g_free (string);
          goto failed;
        }
      else
        {
          have_shape = TRUE;
        }
    }

  g_free (string);

  /* read name */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;

  g_strstrip (string);

  /* the empty string is not an allowed name */
  if (strlen (string) < 1)
    g_strlcpy (string, _("Untitled"), sizeof (string));

  name = gimp_any_to_utf8 (string, -1,
                           _("Invalid UTF-8 string in brush file '%s'."),
                           gimp_file_get_utf8_name (file));

  g_free (string);

  if (have_shape)
    {
      GEnumClass *enum_class;
      GEnumValue *shape_val;

      enum_class = g_type_class_peek (GIMP_TYPE_BRUSH_GENERATED_SHAPE);

      /* read shape */
      linenum++;
      string_len = 256;
      string = g_data_input_stream_read_line (data_input, &string_len,
                                              NULL, &my_error);
      if (! string)
        goto failed;

      g_strstrip (string);
      shape_val = g_enum_get_value_by_nick (enum_class, string);

      if (! shape_val)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "Unknown GIMP brush shape in line %d."),
                       gimp_file_get_utf8_name (file), linenum);
          g_free (string);
          goto failed;
        }

      g_free (string);

      shape = shape_val->value;
    }

  /* read brush spacing */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;
  spacing = g_ascii_strtod (string, NULL);
  g_free (string);

  /* read brush radius */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;
  radius = g_ascii_strtod (string, NULL);
  g_free (string);

  if (have_shape)
    {
      /* read number of spikes */
      linenum++;
      string_len = 256;
      string = g_data_input_stream_read_line (data_input, &string_len,
                                              NULL, &my_error);
      if (! string)
        goto failed;
      spikes = CLAMP (atoi (string), 2, 20);
      g_free (string);
    }

  /* read brush hardness */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;
  hardness = g_ascii_strtod (string, NULL);
  g_free (string);

  /* read brush aspect_ratio */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;
  aspect_ratio = g_ascii_strtod (string, NULL);
  g_free (string);

  /* read brush angle */
  linenum++;
  string_len = 256;
  string = g_data_input_stream_read_line (data_input, &string_len,
                                          NULL, &my_error);
  if (! string)
    goto failed;
  angle = g_ascii_strtod (string, NULL);
  g_free (string);

  g_object_unref (data_input);

  brush = GIMP_BRUSH (gimp_brush_generated_new (name, shape, radius, spikes,
                                                hardness, aspect_ratio, angle));
  g_free (name);

  brush->spacing = spacing;

  return g_list_prepend (NULL, brush);

 failed:

  g_object_unref (data_input);

  if (name)
    g_free (name);

  if (error && *error == NULL)
    {
      gchar *msg;

      if (my_error)
        msg = g_strdup_printf (_("Line %d: %s"), linenum, my_error->message);
      else
        msg = g_strdup_printf (_("File is truncated in line %d"), linenum);

      g_clear_error (&my_error);

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Error while reading brush file '%s': %s"),
                   gimp_file_get_utf8_name (file), msg);

      g_free (msg);
    }

  return NULL;
}
