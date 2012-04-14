/*
 * Copyright 2012 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-plugin.h"
#include "gimp-operation-histogram-sink.h"
#include "gegl-utils.h"
#include "buffer/gegl-buffer.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_AUX
};

static void     get_property (GObject             *gobject,
                              guint                prop_id,
                              GValue              *value,
                              GParamSpec          *pspec);
static void     set_property (GObject             *gobject,
                              guint                prop_id,
                              const GValue        *value,
                              GParamSpec          *pspec);
static gboolean gimp_operation_histogram_sink_process (GeglOperation       *operation,
                              GeglOperationContext     *context,
                              const gchar         *output_prop,
                              const GeglRectangle *result,
                              gint                 level);
static void     attach       (GeglOperation       *operation);

static GeglRectangle get_required_for_output (GeglOperation        *self,
                                               const gchar         *input_pad,
                                               const GeglRectangle *roi);

G_DEFINE_TYPE (GimpOperationHistogramSink, gimp_operation_histogram_sink,
               GEGL_TYPE_OPERATION_SINK)


static void
gimp_operation_histogram_sink_class_init (GimpOperationHistogramSinkClass * klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  operation_class->process = gimp_operation_histogram_sink_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->attach = attach;

  g_object_class_install_property (object_class, PROP_AUX,
                                   g_param_spec_object ("aux",
                                                        "Input",
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
attach (GeglOperation *self)
{
  GeglOperation *operation    = GEGL_OPERATION (self);
  GObjectClass  *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "aux"));
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
}

static gboolean
gimp_operation_histogram_sink_process (GeglOperation        *operation,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level)
{
  GeglBuffer                 *input;
  GeglBuffer                 *aux;
  gboolean                    success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a composer", output_prop);
      return FALSE;
    }

  input = gegl_operation_context_get_source (context, "input");
  aux   = gegl_operation_context_get_source (context, "aux");

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux != NULL)
    {
      /* shake and stir bits */
    }
  else
    {
      g_warning ("received NULL input and aux");
    }

  return success;
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  /* dunno what to do here, make a wild guess */
  return *roi;
}
