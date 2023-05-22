/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpcurve.h"
#include "gimpdynamics.h"
#include "gimpdynamics-load.h"
#include "gimpdynamics-save.h"
#include "gimpdynamicsoutput.h"

#include "gimp-intl.h"


#define DEFAULT_NAME "Nameless dynamics"

enum
{
  PROP_0,

  PROP_NAME,

  PROP_OPACITY_OUTPUT,
  PROP_SIZE_OUTPUT,
  PROP_ANGLE_OUTPUT,
  PROP_COLOR_OUTPUT,
  PROP_FORCE_OUTPUT,
  PROP_HARDNESS_OUTPUT,
  PROP_ASPECT_RATIO_OUTPUT,
  PROP_SPACING_OUTPUT,
  PROP_RATE_OUTPUT,
  PROP_FLOW_OUTPUT,
  PROP_JITTER_OUTPUT
};


typedef struct _GimpDynamicsPrivate GimpDynamicsPrivate;

struct _GimpDynamicsPrivate
{
  GimpDynamicsOutput *opacity_output;
  GimpDynamicsOutput *hardness_output;
  GimpDynamicsOutput *force_output;
  GimpDynamicsOutput *rate_output;
  GimpDynamicsOutput *flow_output;
  GimpDynamicsOutput *size_output;
  GimpDynamicsOutput *aspect_ratio_output;
  GimpDynamicsOutput *color_output;
  GimpDynamicsOutput *angle_output;
  GimpDynamicsOutput *jitter_output;
  GimpDynamicsOutput *spacing_output;
};

#define GET_PRIVATE(output) \
        ((GimpDynamicsPrivate *) gimp_dynamics_get_instance_private ((GimpDynamics *) (output)))


static void          gimp_dynamics_finalize      (GObject      *object);
static void          gimp_dynamics_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void          gimp_dynamics_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void
       gimp_dynamics_dispatch_properties_changed (GObject      *object,
                                                  guint         n_pspecs,
                                                  GParamSpec  **pspecs);

static const gchar * gimp_dynamics_get_extension (GimpData     *data);
static void          gimp_dynamics_copy          (GimpData     *data,
                                                  GimpData     *src_data);

static GimpDynamicsOutput *
                     gimp_dynamics_create_output (GimpDynamics           *dynamics,
                                                  const gchar            *name,
                                                  GimpDynamicsOutputType  type);
static void          gimp_dynamics_output_notify (GObject          *output,
                                                  const GParamSpec *pspec,
                                                  GimpDynamics     *dynamics);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDynamics, gimp_dynamics, GIMP_TYPE_DATA)

#define parent_class gimp_dynamics_parent_class


static void
gimp_dynamics_class_init (GimpDynamicsClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpDataClass     *data_class     = GIMP_DATA_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize                    = gimp_dynamics_finalize;
  object_class->set_property                = gimp_dynamics_set_property;
  object_class->get_property                = gimp_dynamics_get_property;
  object_class->dispatch_properties_changed = gimp_dynamics_dispatch_properties_changed;

  viewable_class->default_icon_name         = "gimp-dynamics";

  data_class->save                          = gimp_dynamics_save;
  data_class->get_extension                 = gimp_dynamics_get_extension;
  data_class->copy                          = gimp_dynamics_copy;

  GIMP_CONFIG_PROP_STRING (object_class, PROP_NAME,
                           "name",
                           NULL, NULL,
                           DEFAULT_NAME,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_OPACITY_OUTPUT,
                           "opacity-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FORCE_OUTPUT,
                           "force-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_HARDNESS_OUTPUT,
                           "hardness-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_RATE_OUTPUT,
                           "rate-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FLOW_OUTPUT,
                           "flow-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_SIZE_OUTPUT,
                           "size-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_ASPECT_RATIO_OUTPUT,
                           "aspect-ratio-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_COLOR_OUTPUT,
                           "color-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_ANGLE_OUTPUT,
                           "angle-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_JITTER_OUTPUT,
                           "jitter-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_SPACING_OUTPUT,
                           "spacing-output",
                           NULL, NULL,
                           GIMP_TYPE_DYNAMICS_OUTPUT,
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_dynamics_init (GimpDynamics *dynamics)
{
  GimpDynamicsPrivate *private = GET_PRIVATE (dynamics);

  private->opacity_output =
    gimp_dynamics_create_output (dynamics,
                                 "opacity-output",
                                 GIMP_DYNAMICS_OUTPUT_OPACITY);

  private->force_output =
    gimp_dynamics_create_output (dynamics,
                                 "force-output",
                                 GIMP_DYNAMICS_OUTPUT_FORCE);

  private->hardness_output =
    gimp_dynamics_create_output (dynamics,
                                 "hardness-output",
                                 GIMP_DYNAMICS_OUTPUT_HARDNESS);

  private->rate_output =
    gimp_dynamics_create_output (dynamics,
                                 "rate-output",
                                 GIMP_DYNAMICS_OUTPUT_RATE);

  private->flow_output =
    gimp_dynamics_create_output (dynamics,
                                 "flow-output",
                                 GIMP_DYNAMICS_OUTPUT_FLOW);

  private->size_output =
    gimp_dynamics_create_output (dynamics,
                                 "size-output",
                                 GIMP_DYNAMICS_OUTPUT_SIZE);

  private->aspect_ratio_output =
    gimp_dynamics_create_output (dynamics,
                                 "aspect-ratio-output",
                                 GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO);

  private->color_output =
    gimp_dynamics_create_output (dynamics,
                                 "color-output",
                                 GIMP_DYNAMICS_OUTPUT_COLOR);

  private->angle_output =
    gimp_dynamics_create_output (dynamics,
                                 "angle-output",
                                 GIMP_DYNAMICS_OUTPUT_ANGLE);

  private->jitter_output =
    gimp_dynamics_create_output (dynamics,
                                 "jitter-output",
                                 GIMP_DYNAMICS_OUTPUT_JITTER);

  private->spacing_output =
    gimp_dynamics_create_output (dynamics,
                                 "spacing-output",
                                 GIMP_DYNAMICS_OUTPUT_SPACING);
}

static void
gimp_dynamics_finalize (GObject *object)
{
  GimpDynamicsPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->opacity_output);
  g_clear_object (&private->force_output);
  g_clear_object (&private->hardness_output);
  g_clear_object (&private->rate_output);
  g_clear_object (&private->flow_output);
  g_clear_object (&private->size_output);
  g_clear_object (&private->aspect_ratio_output);
  g_clear_object (&private->color_output);
  g_clear_object (&private->angle_output);
  g_clear_object (&private->jitter_output);
  g_clear_object (&private->spacing_output);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dynamics_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpDynamicsPrivate *private     = GET_PRIVATE (object);
  GimpDynamicsOutput  *src_output  = NULL;
  GimpDynamicsOutput  *dest_output = NULL;

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (GIMP_OBJECT (object), g_value_get_string (value));
      break;

    case PROP_OPACITY_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->opacity_output;
      break;

    case PROP_FORCE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->force_output;
      break;

    case PROP_HARDNESS_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->hardness_output;
      break;

    case PROP_RATE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->rate_output;
      break;

    case PROP_FLOW_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->flow_output;
      break;

    case PROP_SIZE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->size_output;
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->aspect_ratio_output;
      break;

    case PROP_COLOR_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->color_output;
      break;

    case PROP_ANGLE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->angle_output;
      break;

    case PROP_JITTER_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->jitter_output;
      break;

    case PROP_SPACING_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->spacing_output;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (src_output && dest_output)
    {
      gimp_config_copy (GIMP_CONFIG (src_output),
                        GIMP_CONFIG (dest_output),
                        GIMP_CONFIG_PARAM_SERIALIZE);
    }
}

static void
gimp_dynamics_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpDynamicsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gimp_object_get_name (object));
      break;

    case PROP_OPACITY_OUTPUT:
      g_value_set_object (value, private->opacity_output);
      break;

    case PROP_FORCE_OUTPUT:
      g_value_set_object (value, private->force_output);
      break;

    case PROP_HARDNESS_OUTPUT:
      g_value_set_object (value, private->hardness_output);
      break;

    case PROP_RATE_OUTPUT:
      g_value_set_object (value, private->rate_output);
      break;

    case PROP_FLOW_OUTPUT:
      g_value_set_object (value, private->flow_output);
      break;

    case PROP_SIZE_OUTPUT:
      g_value_set_object (value, private->size_output);
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      g_value_set_object (value, private->aspect_ratio_output);
      break;

    case PROP_COLOR_OUTPUT:
      g_value_set_object (value, private->color_output);
      break;

    case PROP_ANGLE_OUTPUT:
      g_value_set_object (value, private->angle_output);
      break;

    case PROP_JITTER_OUTPUT:
      g_value_set_object (value, private->jitter_output);
      break;

    case PROP_SPACING_OUTPUT:
      g_value_set_object (value, private->spacing_output);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dynamics_dispatch_properties_changed (GObject     *object,
                                           guint        n_pspecs,
                                           GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & GIMP_CONFIG_PARAM_SERIALIZE)
        {
          gimp_data_dirty (GIMP_DATA (object));
          break;
        }
    }
}

static const gchar *
gimp_dynamics_get_extension (GimpData *data)
{
  return GIMP_DYNAMICS_FILE_EXTENSION;
}

static void
gimp_dynamics_copy (GimpData *data,
                    GimpData *src_data)
{
  gimp_data_freeze (data);

  gimp_config_copy (GIMP_CONFIG (src_data),
                    GIMP_CONFIG (data), 0);

  gimp_data_thaw (data);
}


/*  public functions  */

GimpData *
gimp_dynamics_new (GimpContext *context,
                   const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\0', NULL);

  return g_object_new (GIMP_TYPE_DYNAMICS,
                       "name", name,
                       NULL);
}

GimpData *
gimp_dynamics_get_standard (GimpContext *context)
{
  static GimpData *standard_dynamics = NULL;

  if (! standard_dynamics)
    {
      g_set_weak_pointer (&standard_dynamics,
                          gimp_dynamics_new (context, "Standard dynamics"));

      gimp_data_clean (standard_dynamics);
      gimp_data_make_internal (standard_dynamics, "gimp-dynamics-standard");
    }

  return standard_dynamics;
}

GimpDynamicsOutput *
gimp_dynamics_get_output (GimpDynamics           *dynamics,
                          GimpDynamicsOutputType  type_id)
{
  GimpDynamicsPrivate *private;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (dynamics), NULL);

  private = GET_PRIVATE (dynamics);

  switch (type_id)
    {
    case GIMP_DYNAMICS_OUTPUT_OPACITY:
      return private->opacity_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_FORCE:
      return private->force_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_HARDNESS:
      return private->hardness_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_RATE:
      return private->rate_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_FLOW:
      return private->flow_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_SIZE:
      return private->size_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO:
      return private->aspect_ratio_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_COLOR:
      return private->color_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_ANGLE:
      return private->angle_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_JITTER:
      return private->jitter_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_SPACING:
      return private->spacing_output;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }
}

gboolean
gimp_dynamics_is_output_enabled (GimpDynamics           *dynamics,
                                 GimpDynamicsOutputType  type)
{
  GimpDynamicsOutput *output;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (dynamics), FALSE);

  output = gimp_dynamics_get_output (dynamics, type);

  return gimp_dynamics_output_is_enabled (output);
}

gdouble
gimp_dynamics_get_linear_value (GimpDynamics           *dynamics,
                                GimpDynamicsOutputType  type,
                                const GimpCoords       *coords,
                                GimpPaintOptions       *options,
                                gdouble                 fade_point)
{
  GimpDynamicsOutput *output;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (dynamics), 0.0);

  output = gimp_dynamics_get_output (dynamics, type);

  return gimp_dynamics_output_get_linear_value (output, coords,
                                                options, fade_point);
}

gdouble
gimp_dynamics_get_angular_value (GimpDynamics           *dynamics,
                                 GimpDynamicsOutputType  type,
                                 const GimpCoords       *coords,
                                 GimpPaintOptions       *options,
                                 gdouble                 fade_point)
{
  GimpDynamicsOutput *output;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (dynamics), 0.0);

  output = gimp_dynamics_get_output (dynamics, type);

  return gimp_dynamics_output_get_angular_value (output, coords,
                                                 options, fade_point);
}

gdouble
gimp_dynamics_get_aspect_value (GimpDynamics           *dynamics,
                                GimpDynamicsOutputType  type,
                                const GimpCoords       *coords,
                                GimpPaintOptions       *options,
                                gdouble                 fade_point)
{
  GimpDynamicsOutput *output;

  g_return_val_if_fail (GIMP_IS_DYNAMICS (dynamics), 0.0);

  output = gimp_dynamics_get_output (dynamics, type);

  return gimp_dynamics_output_get_aspect_value (output, coords,
                                                options, fade_point);
}


/*  private functions  */

static GimpDynamicsOutput *
gimp_dynamics_create_output (GimpDynamics           *dynamics,
                             const gchar            *name,
                             GimpDynamicsOutputType  type)
{
  GimpDynamicsOutput *output = gimp_dynamics_output_new (name, type);

  g_signal_connect (output, "notify",
                    G_CALLBACK (gimp_dynamics_output_notify),
                    dynamics);

  return output;
}

static void
gimp_dynamics_output_notify (GObject          *output,
                             const GParamSpec *pspec,
                             GimpDynamics     *dynamics)
{
  g_object_notify (G_OBJECT (dynamics), gimp_object_get_name (output));
}
