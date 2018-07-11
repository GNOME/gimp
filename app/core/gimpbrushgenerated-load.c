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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-utils.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushgenerated-load.h"

#include "gimp-intl.h"


GList *
gimp_brush_generated_load (GimpContext   *context,
                           GFile         *file,
                           GInputStream  *input,
                           GError       **error)
{
  GimpBrush               *brush;
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

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data_input = g_data_input_stream_new (input);

  /* make sure the file we are reading is the right type */
  linenum = 1;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;

  if (! g_str_has_prefix (string, "GIMP-VBR"))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Not a GIMP brush file."));
      g_free (string);
      goto failed;
    }

  g_free (string);

  /* make sure we are reading a compatible version */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;

  if (! g_str_has_prefix (string, "1.0"))
    {
      if (! g_str_has_prefix (string, "1.5"))
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Unknown GIMP brush version."));
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
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;

  g_strstrip (string);

  /* the empty string is not an allowed name */
  if (strlen (string) < 1)
    {
      name = g_strdup (_("Untitled"));
    }
  else
    {
      name = gimp_any_to_utf8 (string, -1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               gimp_file_get_utf8_name (file));
    }

  g_free (string);

  if (have_shape)
    {
      GEnumClass *enum_class;
      GEnumValue *shape_val;

      enum_class = g_type_class_peek (GIMP_TYPE_BRUSH_GENERATED_SHAPE);

      /* read shape */
      linenum++;
      string_len = 256;
      string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                        NULL, error);
      if (! string)
        goto failed;

      g_strstrip (string);
      shape_val = g_enum_get_value_by_nick (enum_class, string);

      if (! shape_val)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Unknown GIMP brush shape."));
          g_free (string);
          goto failed;
        }

      g_free (string);

      shape = shape_val->value;
    }

  /* read brush spacing */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;
  if (! gimp_ascii_strtod (string, NULL, &spacing))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid brush spacing."));
      g_free (string);
      goto failed;
    }
  g_free (string);


  /* read brush radius */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;
  if (! gimp_ascii_strtod (string, NULL, &radius))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid brush radius."));
      g_free (string);
      goto failed;
    }
  g_free (string);

  if (have_shape)
    {
      /* read number of spikes */
      linenum++;
      string_len = 256;
      string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                        NULL, error);
      if (! string)
        goto failed;
      if (! gimp_ascii_strtoi (string, NULL, 10, &spikes) ||
          spikes < 2 || spikes > 20)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Invalid brush spike count."));
          g_free (string);
          goto failed;
        }
      g_free (string);
    }

  /* read brush hardness */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;
  if (! gimp_ascii_strtod (string, NULL, &hardness))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid brush hardness."));
      g_free (string);
      goto failed;
    }
  g_free (string);

  /* read brush aspect_ratio */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;
  if (! gimp_ascii_strtod (string, NULL, &aspect_ratio))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid brush aspect ratio."));
      g_free (string);
      goto failed;
    }
  g_free (string);

  /* read brush angle */
  linenum++;
  string_len = 256;
  string = gimp_data_input_stream_read_line_always (data_input, &string_len,
                                                    NULL, error);
  if (! string)
    goto failed;
  if (! gimp_ascii_strtod (string, NULL, &angle))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Invalid brush angle."));
      g_free (string);
      goto failed;
    }
  g_free (string);

  g_object_unref (data_input);

  brush = GIMP_BRUSH (gimp_brush_generated_new (name, shape, radius, spikes,
                                                hardness, aspect_ratio, angle));
  g_free (name);

  gimp_brush_set_spacing (brush, spacing);

  return g_list_prepend (NULL, brush);

 failed:

  g_object_unref (data_input);

  if (name)
    g_free (name);

  g_prefix_error (error, _("In line %d of brush file: "), linenum);

  return NULL;
}
