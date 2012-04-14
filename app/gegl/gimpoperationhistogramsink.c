/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhistogramsink.c
 * Copyright (C) 2012 Øyvind Kolås
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

#include "gimp-gegl-types.h"

#include "gimpoperationhistogramsink.h"


enum
{
  PROP_0,
  PROP_INPUT,
  PROP_AUX
};

static void     gimp_operation_histogram_sink_get_property (GObject             *gobject,
                                                            guint                prop_id,
                                                            GValue              *value,
                                                            GParamSpec          *pspec);
static void     gimp_operation_histogram_sink_set_property (GObject             *gobject,
                                                            guint                prop_id,
                                                            const GValue        *value,
                                                            GParamSpec          *pspec);
static gboolean gimp_operation_histogram_sink_process      (GeglOperation       *operation,
                                                            GeglOperationContext     *context,
                                                            const gchar         *output_prop,
                                                            const GeglRectangle *result,
                                                            gint                 level);
static void     gimp_operation_histogram_sink_attach       (GeglOperation       *operation);

static GeglRectangle gimp_operation_histogram_sink_get_required_for_output (GeglOperation        *self,
                                                                            const gchar         *input_pad,
                                                                            const GeglRectangle *roi);

G_DEFINE_TYPE (GimpOperationHistogramSink, gimp_operation_histogram_sink,
               GEGL_TYPE_OPERATION_SINK)


static void
gimp_operation_histogram_sink_class_init (GimpOperationHistogramSinkClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = gimp_operation_histogram_sink_set_property;
  object_class->get_property = gimp_operation_histogram_sink_get_property;

  operation_class->attach                  = gimp_operation_histogram_sink_attach;
  operation_class->get_required_for_output = gimp_operation_histogram_sink_get_required_for_output;
  operation_class->process                 = gimp_operation_histogram_sink_process;

  g_object_class_install_property (object_class, PROP_AUX,
                                   g_param_spec_object ("aux",
                                                        "Aux",
                                                        "Auxiliary image buffer input pad.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));
}

static void
gimp_operation_histogram_sink_init (GimpOperationHistogramSink *self)
{
}

static void
gimp_operation_histogram_sink_get_property (GObject    *object,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
}

static void
gimp_operation_histogram_sink_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
}

static void
gimp_operation_histogram_sink_attach (GeglOperation *self)
{
  GeglOperation *operation    = GEGL_OPERATION (self);
  GObjectClass  *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "aux"));
}

static GeglRectangle
gimp_operation_histogram_sink_get_required_for_output (GeglOperation       *self,
                                                       const gchar         *input_pad,
                                                       const GeglRectangle *roi)
{
  /* dunno what to do here, make a wild guess */
  return *roi;
}

static gboolean
gimp_operation_histogram_sink_process (GeglOperation        *operation,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level)
{
  GeglBuffer *input;
  GeglBuffer *aux;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a sink", output_prop);
      return FALSE;
    }

  input = gegl_operation_context_get_source (context, "input");
  aux   = gegl_operation_context_get_source (context, "aux");

  if (input)
    {
      if (aux)
        {
          /* do hist with mask */
        }
      else
        {
          /* without */
        }

      return TRUE;
    }

  g_warning ("received NULL input");

  return FALSE;
}
