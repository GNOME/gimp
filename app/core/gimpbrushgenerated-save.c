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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpbrushgenerated.h"
#include "gimpbrushgenerated-save.h"

#include "gimp-intl.h"


gboolean
gimp_brush_generated_save (GimpData       *data,
                           GOutputStream  *output,
                           GError        **error)
{
  GimpBrushGenerated *brush = GIMP_BRUSH_GENERATED (data);
  const gchar        *name  = gimp_object_get_name (data);
  GString            *string;
  gchar               buf[G_ASCII_DTOSTR_BUF_SIZE];
  gboolean            have_shape = FALSE;

  g_return_val_if_fail (name != NULL && *name != '\0', FALSE);

  /* write magic header */
  string = g_string_new ("GIMP-VBR\n");

  /* write version */
  if (brush->shape != GIMP_BRUSH_GENERATED_CIRCLE || brush->spikes > 2)
    {
      g_string_append (string, "1.5\n");
      have_shape = TRUE;
    }
  else
    {
      g_string_append (string, "1.0\n");
    }

  /* write name */
  g_string_append_printf (string, "%.255s\n", name);

  if (have_shape)
    {
      GEnumClass *enum_class;
      GEnumValue *shape_val;

      enum_class = g_type_class_peek (GIMP_TYPE_BRUSH_GENERATED_SHAPE);

      /* write shape */
      shape_val = g_enum_get_value (enum_class, brush->shape);
      g_string_append_printf (string, "%s\n", shape_val->value_nick);
    }

  /* write brush spacing */
  g_string_append_printf (string, "%s\n",
                          g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                          gimp_brush_get_spacing (GIMP_BRUSH (brush))));

  /* write brush radius */
  g_string_append_printf (string, "%s\n",
                          g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                          brush->radius));

  if (have_shape)
    {
      /* write brush spikes */
      g_string_append_printf (string, "%d\n", brush->spikes);
    }

  /* write brush hardness */
  g_string_append_printf (string, "%s\n",
                          g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                          brush->hardness));

  /* write brush aspect_ratio */
  g_string_append_printf (string, "%s\n",
                          g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                          brush->aspect_ratio));

  /* write brush angle */
  g_string_append_printf (string, "%s\n",
                          g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                          brush->angle));

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}
