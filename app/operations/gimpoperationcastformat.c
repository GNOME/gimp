/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcastformat.c
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "operations-types.h"

#include "gimpoperationcastformat.h"


enum
{
  PROP_0,
  PROP_INPUT_FORMAT,
  PROP_OUTPUT_FORMAT
};


static void     gimp_operation_cast_format_get_property (GObject              *object,
                                                         guint                 prop_id,
                                                         GValue               *value,
                                                         GParamSpec           *pspec);
static void     gimp_operation_cast_format_set_property (GObject              *object,
                                                         guint                 prop_id,
                                                         const GValue         *value,
                                                         GParamSpec           *pspec);

static void     gimp_operation_cast_format_prepare      (GeglOperation        *operation);
static gboolean gimp_operation_cast_format_process      (GeglOperation        *operation,
                                                         GeglOperationContext *context,
                                                         const gchar          *output_prop,
                                                         const GeglRectangle  *result,
                                                         gint                  level);


G_DEFINE_TYPE (GimpOperationCastFormat, gimp_operation_cast_format,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_cast_format_parent_class


static void
gimp_operation_cast_format_class_init (GimpOperationCastFormatClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = gimp_operation_cast_format_set_property;
  object_class->get_property = gimp_operation_cast_format_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name"       , "gimp:cast-format",
                                 "categories" , "color",
                                 "description", "GIMP Format casting operation",
                                 NULL);

  operation_class->prepare = gimp_operation_cast_format_prepare;
  operation_class->process = gimp_operation_cast_format_process;

  g_object_class_install_property (object_class, PROP_INPUT_FORMAT,
                                   g_param_spec_pointer ("input-format",
                                                         "Input Format",
                                                         "Input Format",
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OUTPUT_FORMAT,
                                   g_param_spec_pointer ("output-format",
                                                         "Output Format",
                                                         "Output Format",
                                                         G_PARAM_READWRITE));
}

static void
gimp_operation_cast_format_init (GimpOperationCastFormat *self)
{
}

static void
gimp_operation_cast_format_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpOperationCastFormat *cast = GIMP_OPERATION_CAST_FORMAT (object);

  switch (prop_id)
    {
    case PROP_INPUT_FORMAT:
      g_value_set_pointer (value, (gpointer) cast->input_format);
      break;

    case PROP_OUTPUT_FORMAT:
      g_value_set_pointer (value, (gpointer) cast->output_format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_operation_cast_format_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpOperationCastFormat *cast = GIMP_OPERATION_CAST_FORMAT (object);

  switch (prop_id)
    {
    case PROP_INPUT_FORMAT:
      cast->input_format = g_value_get_pointer (value);
      break;

    case PROP_OUTPUT_FORMAT:
      cast->output_format = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_operation_cast_format_prepare (GeglOperation *operation)
{
  GimpOperationCastFormat *cast = GIMP_OPERATION_CAST_FORMAT (operation);

  if (cast->input_format)
    gegl_operation_set_format (operation, "input", cast->input_format);

  if (cast->output_format)
    gegl_operation_set_format (operation, "output", cast->output_format);
}

static gboolean
gimp_operation_cast_format_process (GeglOperation        *operation,
                                    GeglOperationContext *context,
                                    const gchar          *output_prop,
                                    const GeglRectangle  *result,
                                    gint                  level)
{
  GimpOperationCastFormat *cast = GIMP_OPERATION_CAST_FORMAT (operation);
  GeglBuffer              *input;
  GeglBuffer              *output;

  if (! cast->input_format || ! cast->output_format)
    {
      g_warning ("cast: input-format or output-format are not set");
      return FALSE;
    }

  if (babl_format_get_bytes_per_pixel (cast->input_format) !=
      babl_format_get_bytes_per_pixel (cast->output_format))
    {
      g_warning ("cast: input-format and output-format have different bpp");
      return FALSE;
    }

  if (strcmp (output_prop, "output"))
    {
      g_warning ("cast: requested processing of %s pad on a sink", output_prop);
      return FALSE;
    }

  input = gegl_operation_context_get_source (context, "input");
  if (! input)
    {
      g_warning ("cast: received NULL input");
      return FALSE;
    }

  output = gegl_buffer_new (result, cast->input_format);

  gegl_buffer_copy (input,  result,
                    output, result);
  gegl_buffer_set_format (output, cast->output_format);

  g_object_unref (input);

  gegl_operation_context_take_object (context, "output", G_OBJECT (output));

  return TRUE;
}
