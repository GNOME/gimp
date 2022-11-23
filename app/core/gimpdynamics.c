/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmacurve.h"
#include "ligmadynamics.h"
#include "ligmadynamics-load.h"
#include "ligmadynamics-save.h"
#include "ligmadynamicsoutput.h"

#include "ligma-intl.h"


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


typedef struct _LigmaDynamicsPrivate LigmaDynamicsPrivate;

struct _LigmaDynamicsPrivate
{
  LigmaDynamicsOutput *opacity_output;
  LigmaDynamicsOutput *hardness_output;
  LigmaDynamicsOutput *force_output;
  LigmaDynamicsOutput *rate_output;
  LigmaDynamicsOutput *flow_output;
  LigmaDynamicsOutput *size_output;
  LigmaDynamicsOutput *aspect_ratio_output;
  LigmaDynamicsOutput *color_output;
  LigmaDynamicsOutput *angle_output;
  LigmaDynamicsOutput *jitter_output;
  LigmaDynamicsOutput *spacing_output;
};

#define GET_PRIVATE(output) \
        ((LigmaDynamicsPrivate *) ligma_dynamics_get_instance_private ((LigmaDynamics *) (output)))


static void          ligma_dynamics_finalize      (GObject      *object);
static void          ligma_dynamics_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void          ligma_dynamics_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void
       ligma_dynamics_dispatch_properties_changed (GObject      *object,
                                                  guint         n_pspecs,
                                                  GParamSpec  **pspecs);

static const gchar * ligma_dynamics_get_extension (LigmaData     *data);
static void          ligma_dynamics_copy          (LigmaData     *data,
                                                  LigmaData     *src_data);

static LigmaDynamicsOutput *
                     ligma_dynamics_create_output (LigmaDynamics           *dynamics,
                                                  const gchar            *name,
                                                  LigmaDynamicsOutputType  type);
static void          ligma_dynamics_output_notify (GObject          *output,
                                                  const GParamSpec *pspec,
                                                  LigmaDynamics     *dynamics);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDynamics, ligma_dynamics, LIGMA_TYPE_DATA)

#define parent_class ligma_dynamics_parent_class


static void
ligma_dynamics_class_init (LigmaDynamicsClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaDataClass     *data_class     = LIGMA_DATA_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->finalize                    = ligma_dynamics_finalize;
  object_class->set_property                = ligma_dynamics_set_property;
  object_class->get_property                = ligma_dynamics_get_property;
  object_class->dispatch_properties_changed = ligma_dynamics_dispatch_properties_changed;

  viewable_class->default_icon_name         = "ligma-dynamics";

  data_class->save                          = ligma_dynamics_save;
  data_class->get_extension                 = ligma_dynamics_get_extension;
  data_class->copy                          = ligma_dynamics_copy;

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_NAME,
                           "name",
                           NULL, NULL,
                           DEFAULT_NAME,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_OPACITY_OUTPUT,
                           "opacity-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_FORCE_OUTPUT,
                           "force-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_HARDNESS_OUTPUT,
                           "hardness-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_RATE_OUTPUT,
                           "rate-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_FLOW_OUTPUT,
                           "flow-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_SIZE_OUTPUT,
                           "size-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_ASPECT_RATIO_OUTPUT,
                           "aspect-ratio-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_COLOR_OUTPUT,
                           "color-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_ANGLE_OUTPUT,
                           "angle-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_JITTER_OUTPUT,
                           "jitter-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_SPACING_OUTPUT,
                           "spacing-output",
                           NULL, NULL,
                           LIGMA_TYPE_DYNAMICS_OUTPUT,
                           LIGMA_CONFIG_PARAM_AGGREGATE);
}

static void
ligma_dynamics_init (LigmaDynamics *dynamics)
{
  LigmaDynamicsPrivate *private = GET_PRIVATE (dynamics);

  private->opacity_output =
    ligma_dynamics_create_output (dynamics,
                                 "opacity-output",
                                 LIGMA_DYNAMICS_OUTPUT_OPACITY);

  private->force_output =
    ligma_dynamics_create_output (dynamics,
                                 "force-output",
                                 LIGMA_DYNAMICS_OUTPUT_FORCE);

  private->hardness_output =
    ligma_dynamics_create_output (dynamics,
                                 "hardness-output",
                                 LIGMA_DYNAMICS_OUTPUT_HARDNESS);

  private->rate_output =
    ligma_dynamics_create_output (dynamics,
                                 "rate-output",
                                 LIGMA_DYNAMICS_OUTPUT_RATE);

  private->flow_output =
    ligma_dynamics_create_output (dynamics,
                                 "flow-output",
                                 LIGMA_DYNAMICS_OUTPUT_FLOW);

  private->size_output =
    ligma_dynamics_create_output (dynamics,
                                 "size-output",
                                 LIGMA_DYNAMICS_OUTPUT_SIZE);

  private->aspect_ratio_output =
    ligma_dynamics_create_output (dynamics,
                                 "aspect-ratio-output",
                                 LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO);

  private->color_output =
    ligma_dynamics_create_output (dynamics,
                                 "color-output",
                                 LIGMA_DYNAMICS_OUTPUT_COLOR);

  private->angle_output =
    ligma_dynamics_create_output (dynamics,
                                 "angle-output",
                                 LIGMA_DYNAMICS_OUTPUT_ANGLE);

  private->jitter_output =
    ligma_dynamics_create_output (dynamics,
                                 "jitter-output",
                                 LIGMA_DYNAMICS_OUTPUT_JITTER);

  private->spacing_output =
    ligma_dynamics_create_output (dynamics,
                                 "spacing-output",
                                 LIGMA_DYNAMICS_OUTPUT_SPACING);
}

static void
ligma_dynamics_finalize (GObject *object)
{
  LigmaDynamicsPrivate *private = GET_PRIVATE (object);

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
ligma_dynamics_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LigmaDynamicsPrivate *private     = GET_PRIVATE (object);
  LigmaDynamicsOutput  *src_output  = NULL;
  LigmaDynamicsOutput  *dest_output = NULL;

  switch (property_id)
    {
    case PROP_NAME:
      ligma_object_set_name (LIGMA_OBJECT (object), g_value_get_string (value));
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
      ligma_config_copy (LIGMA_CONFIG (src_output),
                        LIGMA_CONFIG (dest_output),
                        LIGMA_CONFIG_PARAM_SERIALIZE);
    }
}

static void
ligma_dynamics_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaDynamicsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, ligma_object_get_name (object));
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
ligma_dynamics_dispatch_properties_changed (GObject     *object,
                                           guint        n_pspecs,
                                           GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & LIGMA_CONFIG_PARAM_SERIALIZE)
        {
          ligma_data_dirty (LIGMA_DATA (object));
          break;
        }
    }
}

static const gchar *
ligma_dynamics_get_extension (LigmaData *data)
{
  return LIGMA_DYNAMICS_FILE_EXTENSION;
}

static void
ligma_dynamics_copy (LigmaData *data,
                    LigmaData *src_data)
{
  ligma_data_freeze (data);

  ligma_config_copy (LIGMA_CONFIG (src_data),
                    LIGMA_CONFIG (data), 0);

  ligma_data_thaw (data);
}


/*  public functions  */

LigmaData *
ligma_dynamics_new (LigmaContext *context,
                   const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\0', NULL);

  return g_object_new (LIGMA_TYPE_DYNAMICS,
                       "name", name,
                       NULL);
}

LigmaData *
ligma_dynamics_get_standard (LigmaContext *context)
{
  static LigmaData *standard_dynamics = NULL;

  if (! standard_dynamics)
    {
      standard_dynamics = ligma_dynamics_new (context, "Standard dynamics");

      ligma_data_clean (standard_dynamics);
      ligma_data_make_internal (standard_dynamics, "ligma-dynamics-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_dynamics),
                                 (gpointer *) &standard_dynamics);
    }

  return standard_dynamics;
}

LigmaDynamicsOutput *
ligma_dynamics_get_output (LigmaDynamics           *dynamics,
                          LigmaDynamicsOutputType  type_id)
{
  LigmaDynamicsPrivate *private;

  g_return_val_if_fail (LIGMA_IS_DYNAMICS (dynamics), NULL);

  private = GET_PRIVATE (dynamics);

  switch (type_id)
    {
    case LIGMA_DYNAMICS_OUTPUT_OPACITY:
      return private->opacity_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_FORCE:
      return private->force_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_HARDNESS:
      return private->hardness_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_RATE:
      return private->rate_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_FLOW:
      return private->flow_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_SIZE:
      return private->size_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_ASPECT_RATIO:
      return private->aspect_ratio_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_COLOR:
      return private->color_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_ANGLE:
      return private->angle_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_JITTER:
      return private->jitter_output;
      break;

    case LIGMA_DYNAMICS_OUTPUT_SPACING:
      return private->spacing_output;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }
}

gboolean
ligma_dynamics_is_output_enabled (LigmaDynamics           *dynamics,
                                 LigmaDynamicsOutputType  type)
{
  LigmaDynamicsOutput *output;

  g_return_val_if_fail (LIGMA_IS_DYNAMICS (dynamics), FALSE);

  output = ligma_dynamics_get_output (dynamics, type);

  return ligma_dynamics_output_is_enabled (output);
}

gdouble
ligma_dynamics_get_linear_value (LigmaDynamics           *dynamics,
                                LigmaDynamicsOutputType  type,
                                const LigmaCoords       *coords,
                                LigmaPaintOptions       *options,
                                gdouble                 fade_point)
{
  LigmaDynamicsOutput *output;

  g_return_val_if_fail (LIGMA_IS_DYNAMICS (dynamics), 0.0);

  output = ligma_dynamics_get_output (dynamics, type);

  return ligma_dynamics_output_get_linear_value (output, coords,
                                                options, fade_point);
}

gdouble
ligma_dynamics_get_angular_value (LigmaDynamics           *dynamics,
                                 LigmaDynamicsOutputType  type,
                                 const LigmaCoords       *coords,
                                 LigmaPaintOptions       *options,
                                 gdouble                 fade_point)
{
  LigmaDynamicsOutput *output;

  g_return_val_if_fail (LIGMA_IS_DYNAMICS (dynamics), 0.0);

  output = ligma_dynamics_get_output (dynamics, type);

  return ligma_dynamics_output_get_angular_value (output, coords,
                                                 options, fade_point);
}

gdouble
ligma_dynamics_get_aspect_value (LigmaDynamics           *dynamics,
                                LigmaDynamicsOutputType  type,
                                const LigmaCoords       *coords,
                                LigmaPaintOptions       *options,
                                gdouble                 fade_point)
{
  LigmaDynamicsOutput *output;

  g_return_val_if_fail (LIGMA_IS_DYNAMICS (dynamics), 0.0);

  output = ligma_dynamics_get_output (dynamics, type);

  return ligma_dynamics_output_get_aspect_value (output, coords,
                                                options, fade_point);
}


/*  private functions  */

static LigmaDynamicsOutput *
ligma_dynamics_create_output (LigmaDynamics           *dynamics,
                             const gchar            *name,
                             LigmaDynamicsOutputType  type)
{
  LigmaDynamicsOutput *output = ligma_dynamics_output_new (name, type);

  g_signal_connect (output, "notify",
                    G_CALLBACK (ligma_dynamics_output_notify),
                    dynamics);

  return output;
}

static void
ligma_dynamics_output_notify (GObject          *output,
                             const GParamSpec *pspec,
                             LigmaDynamics     *dynamics)
{
  g_object_notify (G_OBJECT (dynamics), ligma_object_get_name (output));
}
