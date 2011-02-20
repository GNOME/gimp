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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

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
static GimpData *    gimp_dynamics_duplicate     (GimpData     *data);

static GimpDynamicsOutput *
                     gimp_dynamics_create_output (GimpDynamics           *dynamics,
                                                  const gchar            *name,
                                                  GimpDynamicsOutputType  type);
static void          gimp_dynamics_output_notify (GObject          *output,
                                                  const GParamSpec *pspec,
                                                  GimpDynamics     *dynamics);



G_DEFINE_TYPE (GimpDynamics, gimp_dynamics,
               GIMP_TYPE_DATA)

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

  data_class->save                          = gimp_dynamics_save;
  data_class->get_extension                 = gimp_dynamics_get_extension;
  data_class->duplicate                     = gimp_dynamics_duplicate;


  viewable_class->default_stock_id = "gimp-dynamics";

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_NAME,
                                   "name", NULL,
                                   DEFAULT_NAME,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_OPACITY_OUTPUT,
                                   "opacity-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_FORCE_OUTPUT,
                                   "force-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_HARDNESS_OUTPUT,
                                   "hardness-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_RATE_OUTPUT,
                                   "rate-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_FLOW_OUTPUT,
                                   "flow-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_SIZE_OUTPUT,
                                   "size-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_ASPECT_RATIO_OUTPUT,
                                   "aspect-ratio-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_COLOR_OUTPUT,
                                   "color-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_ANGLE_OUTPUT,
                                   "angle-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_JITTER_OUTPUT,
                                   "jitter-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_SPACING_OUTPUT,
                                   "spacing-output", NULL,
                                   GIMP_TYPE_DYNAMICS_OUTPUT,
                                   GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_dynamics_init (GimpDynamics *dynamics)
{
  dynamics->opacity_output      = gimp_dynamics_create_output (dynamics,
                                                               "opacity-output",
                                                               GIMP_DYNAMICS_OUTPUT_OPACITY);

  dynamics->force_output        = gimp_dynamics_create_output (dynamics,
                                                               "force-output",
                                                               GIMP_DYNAMICS_OUTPUT_FORCE);

  dynamics->hardness_output     = gimp_dynamics_create_output (dynamics,
                                                               "hardness-output",
                                                               GIMP_DYNAMICS_OUTPUT_HARDNESS);
  dynamics->rate_output         = gimp_dynamics_create_output (dynamics,
                                                               "rate-output",
                                                               GIMP_DYNAMICS_OUTPUT_RATE);
  dynamics->flow_output         = gimp_dynamics_create_output (dynamics,
                                                               "flow-output",
                                                               GIMP_DYNAMICS_OUTPUT_RATE);
  dynamics->size_output         = gimp_dynamics_create_output (dynamics,
                                                               "size-output",
                                                               GIMP_DYNAMICS_OUTPUT_SIZE);
  dynamics->aspect_ratio_output = gimp_dynamics_create_output (dynamics,
                                                               "aspect-ratio-output",
                                                               GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO);
  dynamics->color_output        = gimp_dynamics_create_output (dynamics,
                                                               "color-output",
                                                               GIMP_DYNAMICS_OUTPUT_COLOR);
  dynamics->angle_output        = gimp_dynamics_create_output (dynamics,
                                                               "angle-output",
                                                               GIMP_DYNAMICS_OUTPUT_ANGLE);
  dynamics->jitter_output       = gimp_dynamics_create_output (dynamics,
                                                               "jitter-output",
                                                               GIMP_DYNAMICS_OUTPUT_JITTER);
  dynamics->spacing_output      = gimp_dynamics_create_output (dynamics,
                                                               "spacing-output",
                                                               GIMP_DYNAMICS_OUTPUT_SPACING);
}

static void
gimp_dynamics_finalize (GObject *object)
{
  GimpDynamics *dynamics = GIMP_DYNAMICS (object);

  g_object_unref (dynamics->opacity_output);
  g_object_unref (dynamics->force_output);
  g_object_unref (dynamics->hardness_output);
  g_object_unref (dynamics->rate_output);
  g_object_unref (dynamics->flow_output);
  g_object_unref (dynamics->size_output);
  g_object_unref (dynamics->aspect_ratio_output);
  g_object_unref (dynamics->color_output);
  g_object_unref (dynamics->angle_output);
  g_object_unref (dynamics->jitter_output);
  g_object_unref (dynamics->spacing_output);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dynamics_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpDynamics       *dynamics    = GIMP_DYNAMICS (object);
  GimpDynamicsOutput *src_output  = NULL;
  GimpDynamicsOutput *dest_output = NULL;

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (GIMP_OBJECT (dynamics), g_value_get_string (value));
      break;

    case PROP_OPACITY_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->opacity_output;
      break;

    case PROP_FORCE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->force_output;
      break;

    case PROP_HARDNESS_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->hardness_output;
      break;

    case PROP_RATE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->rate_output;
      break;

    case PROP_FLOW_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->flow_output;
      break;

    case PROP_SIZE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->size_output;
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->aspect_ratio_output;
      break;

    case PROP_COLOR_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->color_output;
      break;

    case PROP_ANGLE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->angle_output;
      break;

    case PROP_JITTER_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->jitter_output;
      break;

    case PROP_SPACING_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = dynamics->spacing_output;
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
  GimpDynamics *dynamics = GIMP_DYNAMICS (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gimp_object_get_name (dynamics));
      break;

    case PROP_OPACITY_OUTPUT:
      g_value_set_object (value, dynamics->opacity_output);
      break;

    case PROP_FORCE_OUTPUT:
      g_value_set_object (value, dynamics->force_output);
      break;

    case PROP_HARDNESS_OUTPUT:
      g_value_set_object (value, dynamics->hardness_output);
      break;

    case PROP_RATE_OUTPUT:
      g_value_set_object (value, dynamics->rate_output);
      break;

    case PROP_FLOW_OUTPUT:
      g_value_set_object (value, dynamics->flow_output);
      break;

    case PROP_SIZE_OUTPUT:
      g_value_set_object (value, dynamics->size_output);
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      g_value_set_object (value, dynamics->aspect_ratio_output);
      break;

    case PROP_COLOR_OUTPUT:
      g_value_set_object (value, dynamics->color_output);
      break;

    case PROP_ANGLE_OUTPUT:
      g_value_set_object (value, dynamics->angle_output);
      break;

    case PROP_JITTER_OUTPUT:
      g_value_set_object (value, dynamics->jitter_output);
      break;

    case PROP_SPACING_OUTPUT:
      g_value_set_object (value, dynamics->spacing_output);
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

static GimpData *
gimp_dynamics_duplicate (GimpData *data)
{
  GimpData *dest = g_object_new (GIMP_TYPE_DYNAMICS, NULL);

  gimp_config_copy (GIMP_CONFIG (data),
                    GIMP_CONFIG (dest), 0);

  return GIMP_DATA (dest);
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
      standard_dynamics = gimp_dynamics_new (context, "Standard dynamics");

      gimp_data_clean (standard_dynamics);
      gimp_data_make_internal (standard_dynamics, "gimp-dynamics-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_dynamics),
                                 (gpointer *) &standard_dynamics);
    }

  return standard_dynamics;
}

GimpDynamicsOutput *
gimp_dynamics_get_output (GimpDynamics           *dynamics,
                          GimpDynamicsOutputType  type_id)
{

  switch (type_id)
    {
    case GIMP_DYNAMICS_OUTPUT_OPACITY:
      return dynamics->opacity_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_FORCE:
      return dynamics->force_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_HARDNESS:
      return dynamics->hardness_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_RATE:
      return dynamics->rate_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_FLOW:
      return dynamics->flow_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_SIZE:
      return dynamics->size_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_ASPECT_RATIO:
      return dynamics->aspect_ratio_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_COLOR:
      return dynamics->color_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_ANGLE:
      return dynamics->angle_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_JITTER:
      return dynamics->jitter_output;
      break;

    case GIMP_DYNAMICS_OUTPUT_SPACING:
      return dynamics->spacing_output;
      break;

    default:
      return NULL;
      break;
    }
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
