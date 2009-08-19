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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpgradient.h"
#include "core/gimppaintinfo.h"

#include "gimpdynamicsoptions.h"

#include "core/gimpdata.h"
#include "gimp-intl.h"


//#define DEFAULT_DYNAMICS_EXPANDED      FALSE

#define DEFAULT_PRESSURE_OPACITY       TRUE
#define DEFAULT_PRESSURE_HARDNESS      FALSE
#define DEFAULT_PRESSURE_RATE          FALSE
#define DEFAULT_PRESSURE_SIZE          FALSE
#define DEFAULT_PRESSURE_INVERSE_SIZE  FALSE
#define DEFAULT_PRESSURE_ASPECT_RATIO  FALSE
#define DEFAULT_PRESSURE_COLOR         FALSE
#define DEFAULT_PRESSURE_ANGLE         FALSE
#define DEFAULT_PRESSURE_PRESCALE      1.0

#define DEFAULT_VELOCITY_OPACITY       FALSE
#define DEFAULT_VELOCITY_HARDNESS      FALSE
#define DEFAULT_VELOCITY_RATE          FALSE
#define DEFAULT_VELOCITY_SIZE          FALSE
#define DEFAULT_VELOCITY_INVERSE_SIZE  FALSE
#define DEFAULT_VELOCITY_ASPECT_RATIO  FALSE
#define DEFAULT_VELOCITY_COLOR         FALSE
#define DEFAULT_VELOCITY_ANGLE         FALSE
#define DEFAULT_VELOCITY_PRESCALE      1.0

#define DEFAULT_DIRECTION_OPACITY      FALSE
#define DEFAULT_DIRECTION_HARDNESS     FALSE
#define DEFAULT_DIRECTION_RATE         FALSE
#define DEFAULT_DIRECTION_SIZE         FALSE
#define DEFAULT_DIRECTION_INVERSE_SIZE FALSE
#define DEFAULT_DIRECTION_ASPECT_RATIO FALSE
#define DEFAULT_DIRECTION_COLOR        FALSE
#define DEFAULT_DIRECTION_ANGLE        FALSE
#define DEFAULT_DIRECTION_PRESCALE     1.0

#define DEFAULT_TILT_OPACITY           FALSE
#define DEFAULT_TILT_HARDNESS          FALSE
#define DEFAULT_TILT_RATE              FALSE
#define DEFAULT_TILT_SIZE              FALSE
#define DEFAULT_TILT_INVERSE_SIZE      FALSE
#define DEFAULT_TILT_ASPECT_RATIO      FALSE
#define DEFAULT_TILT_COLOR             FALSE
#define DEFAULT_TILT_ANGLE             FALSE
#define DEFAULT_TILT_PRESCALE          1.0

#define DEFAULT_RANDOM_OPACITY         FALSE
#define DEFAULT_RANDOM_HARDNESS        FALSE
#define DEFAULT_RANDOM_RATE            FALSE
#define DEFAULT_RANDOM_SIZE            FALSE
#define DEFAULT_RANDOM_INVERSE_SIZE    FALSE
#define DEFAULT_RANDOM_ASPECT_RATIO    FALSE
#define DEFAULT_RANDOM_COLOR           FALSE
#define DEFAULT_RANDOM_ANGLE           FALSE
#define DEFAULT_RANDOM_PRESCALE        1.0

#define DEFAULT_FADING_OPACITY         FALSE
#define DEFAULT_FADING_HARDNESS        FALSE
#define DEFAULT_FADING_RATE            FALSE
#define DEFAULT_FADING_SIZE            FALSE
#define DEFAULT_FADING_INVERSE_SIZE    FALSE
#define DEFAULT_FADING_ASPECT_RATIO    FALSE
#define DEFAULT_FADING_COLOR           FALSE
#define DEFAULT_FADING_ANGLE           FALSE
#define DEFAULT_FADING_PRESCALE        1.0

#define DEFAULT_FADE_LENGTH            100.0

enum
{
  PROP_0,

  PROP_DYNAMICS_INFO,

  PROP_DYNAMICS_EXPANDED,

  PROP_PRESSURE_OPACITY,
  PROP_PRESSURE_HARDNESS,
  PROP_PRESSURE_RATE,
  PROP_PRESSURE_SIZE,
  PROP_PRESSURE_INVERSE_SIZE,
  PROP_PRESSURE_ASPECT_RATIO,
  PROP_PRESSURE_COLOR,
  PROP_PRESSURE_ANGLE,
  PROP_PRESSURE_PRESCALE,

  PROP_VELOCITY_OPACITY,
  PROP_VELOCITY_HARDNESS,
  PROP_VELOCITY_RATE,
  PROP_VELOCITY_SIZE,
  PROP_VELOCITY_INVERSE_SIZE,
  PROP_VELOCITY_ASPECT_RATIO,
  PROP_VELOCITY_COLOR,
  PROP_VELOCITY_ANGLE,
  PROP_VELOCITY_PRESCALE,

  PROP_DIRECTION_OPACITY,
  PROP_DIRECTION_HARDNESS,
  PROP_DIRECTION_RATE,
  PROP_DIRECTION_SIZE,
  PROP_DIRECTION_INVERSE_SIZE,
  PROP_DIRECTION_ASPECT_RATIO,
  PROP_DIRECTION_COLOR,
  PROP_DIRECTION_ANGLE,
  PROP_DIRECTION_PRESCALE,

  PROP_TILT_OPACITY,
  PROP_TILT_HARDNESS,
  PROP_TILT_RATE,
  PROP_TILT_SIZE,
  PROP_TILT_INVERSE_SIZE,
  PROP_TILT_ASPECT_RATIO,
  PROP_TILT_COLOR,
  PROP_TILT_ANGLE,
  PROP_TILT_PRESCALE,

  PROP_RANDOM_OPACITY,
  PROP_RANDOM_HARDNESS,
  PROP_RANDOM_RATE,
  PROP_RANDOM_SIZE,
  PROP_RANDOM_INVERSE_SIZE,
  PROP_RANDOM_ASPECT_RATIO,
  PROP_RANDOM_COLOR,
  PROP_RANDOM_ANGLE,
  PROP_RANDOM_PRESCALE,

  PROP_FADING_OPACITY,
  PROP_FADING_HARDNESS,
  PROP_FADING_RATE,
  PROP_FADING_SIZE,
  PROP_FADING_INVERSE_SIZE,
  PROP_FADING_ASPECT_RATIO,
  PROP_FADING_COLOR,
  PROP_FADING_ANGLE,
  PROP_FADING_PRESCALE,
};

/*
static gdouble gimp_dynamics_options_get_dynamics_mix (gdouble       mix1,
                                                       gdouble       mix1_scale,
                                                       gdouble       mix2,
                                                       gdouble       mix2_scale,
                                                       gdouble       mix3,
                                                       gdouble       mix3_scale,
                                                       gdouble       mix4,
                                                       gdouble       mix4_scale,
                                                       gdouble       mix5,
                                                       gdouble       mix5_scale,
                                                       gdouble       mix6,
                                                       gdouble       mix6_scale);
*/

static void    gimp_dynamics_options_class_init (GimpDynamicsOptionsClass *klass);

static void    gimp_dynamics_options_finalize         (GObject      *object);


static void    gimp_dynamics_options_notify           (GObject      *object,
                                                       GParamSpec   *pspec);

static void    gimp_dynamics_options_set_property     (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void    gimp_dynamics_options_get_property     (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

/*
G_DEFINE_TYPE_WITH_CODE (GimpDynamicsOptions, gimp_dynamics_options, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_dynamics_editor_docked_iface_init))
*/
G_DEFINE_TYPE (GimpDynamicsOptions, gimp_dynamics_options,
               GIMP_TYPE_DATA)


#define parent_class gimp_dynamics_options_parent_class


static void
gimp_dynamics_options_class_init (GimpDynamicsOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_dynamics_options_finalize;
  object_class->set_property = gimp_dynamics_options_set_property;
  object_class->get_property = gimp_dynamics_options_get_property;
  object_class->notify       = gimp_dynamics_options_notify;


  g_object_class_install_property (object_class, PROP_DYNAMICS_INFO,
                                   g_param_spec_object ("dynamics-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PAINT_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_ASPECT_RATIO,
                                    "random-aspect-ratio", NULL,
                                    DEFAULT_RANDOM_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_ASPECT_RATIO,
                                    "fading-aspect-ratio", NULL,
                                    DEFAULT_FADING_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_OPACITY,
                                    "pressure-opacity", NULL,
                                    DEFAULT_PRESSURE_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_HARDNESS,
                                    "pressure-hardness", NULL,
                                    DEFAULT_PRESSURE_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_RATE,
                                    "pressure-rate", NULL,
                                    DEFAULT_PRESSURE_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_SIZE,
                                    "pressure-size", NULL,
                                    DEFAULT_PRESSURE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_COLOR,
                                    "pressure-color", NULL,
                                    DEFAULT_PRESSURE_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_ANGLE,
                                    "pressure-angle", NULL,
                                    DEFAULT_PRESSURE_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_INVERSE_SIZE,
                                    "pressure-inverse-size", NULL,
                                    DEFAULT_PRESSURE_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_ASPECT_RATIO,
                                    "pressure-aspect-ratio", NULL,
                                    DEFAULT_PRESSURE_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_PRESSURE_PRESCALE,
                                   "pressure-prescale", NULL,
                                   0.0, 1.0, DEFAULT_PRESSURE_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_OPACITY,
                                    "velocity-opacity", NULL,
                                    DEFAULT_VELOCITY_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_HARDNESS,
                                    "velocity-hardness", NULL,
                                    DEFAULT_VELOCITY_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_RATE,
                                    "velocity-rate", NULL,
                                    DEFAULT_VELOCITY_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_SIZE,
                                    "velocity-size", NULL,
                                    DEFAULT_VELOCITY_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_COLOR,
                                    "velocity-color", NULL,
                                    DEFAULT_VELOCITY_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_ANGLE,
                                    "velocity-angle", NULL,
                                    DEFAULT_VELOCITY_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_INVERSE_SIZE,
                                    "velocity-inverse-size", NULL,
                                    DEFAULT_VELOCITY_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VELOCITY_ASPECT_RATIO,
                                    "velocity-aspect-ratio", NULL,
                                    DEFAULT_VELOCITY_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_VELOCITY_PRESCALE,
                                   "velocity-prescale", NULL,
                                   0.0, 1.0, DEFAULT_VELOCITY_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_OPACITY,
                                    "direction-opacity", NULL,
                                    DEFAULT_DIRECTION_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_HARDNESS,
                                    "direction-hardness", NULL,
                                    DEFAULT_DIRECTION_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_RATE,
                                    "direction-rate", NULL,
                                    DEFAULT_DIRECTION_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_SIZE,
                                    "direction-size", NULL,
                                    DEFAULT_DIRECTION_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_COLOR,
                                    "direction-color", NULL,
                                    DEFAULT_DIRECTION_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_ANGLE,
                                    "direction-angle", NULL,
                                    DEFAULT_DIRECTION_ANGLE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_INVERSE_SIZE,
                                    "direction-inverse-size", NULL,
                                    DEFAULT_DIRECTION_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DIRECTION_ASPECT_RATIO,
                                    "direction-aspect-ratio", NULL,
                                    DEFAULT_DIRECTION_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_DIRECTION_PRESCALE,
                                   "direction-prescale", NULL,
                                   0.0, 1.0, DEFAULT_DIRECTION_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_OPACITY,
                                    "tilt-opacity", NULL,
                                    DEFAULT_TILT_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_HARDNESS,
                                    "tilt-hardness", NULL,
                                    DEFAULT_TILT_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_RATE,
                                    "tilt-rate", NULL,
                                    DEFAULT_TILT_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_SIZE,
                                    "tilt-size", NULL,
                                    DEFAULT_TILT_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_COLOR,
                                    "tilt-color", NULL,
                                    DEFAULT_TILT_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_ANGLE,
                                    "tilt-angle", NULL,
                                    DEFAULT_TILT_ANGLE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_INVERSE_SIZE,
                                    "tilt-inverse-size", NULL,
                                    DEFAULT_TILT_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_TILT_ASPECT_RATIO,
                                    "tilt-aspect-ratio", NULL,
                                    DEFAULT_TILT_ASPECT_RATIO,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_TILT_PRESCALE,
                                   "tilt-prescale", NULL,
                                   0.0, 1.0, DEFAULT_TILT_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_OPACITY,
                                    "random-opacity", NULL,
                                    DEFAULT_RANDOM_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_HARDNESS,
                                    "random-hardness", NULL,
                                    DEFAULT_RANDOM_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_RATE,
                                    "random-rate", NULL,
                                    DEFAULT_RANDOM_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_SIZE,
                                    "random-size", NULL,
                                    DEFAULT_RANDOM_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_COLOR,
                                    "random-color", NULL,
                                    DEFAULT_RANDOM_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_ANGLE,
                                    "random-angle", NULL,
                                    DEFAULT_RANDOM_ANGLE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_RANDOM_INVERSE_SIZE,
                                    "random-inverse-size", NULL,
                                    DEFAULT_RANDOM_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_RANDOM_PRESCALE,
                                   "random-prescale", NULL,
                                   0.0, 1.0, DEFAULT_RANDOM_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_OPACITY,
                                    "fading-opacity", NULL,
                                    DEFAULT_FADING_OPACITY,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_HARDNESS,
                                    "fading-hardness", NULL,
                                    DEFAULT_FADING_HARDNESS,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_RATE,
                                    "fading-rate", NULL,
                                    DEFAULT_FADING_RATE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_SIZE,
                                    "fading-size", NULL,
                                    DEFAULT_FADING_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_COLOR,
                                    "fading-color", NULL,
                                    DEFAULT_FADING_COLOR,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_ANGLE,
                                    "fading-angle", NULL,
                                    DEFAULT_FADING_ANGLE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FADING_INVERSE_SIZE,
                                    "fading-inverse-size", NULL,
                                    DEFAULT_FADING_INVERSE_SIZE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FADING_PRESCALE,
                                   "fading-prescale", NULL,
                                   0.0, 1.0, DEFAULT_FADING_PRESCALE,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_dynamics_options_init (GimpDynamicsOptions *options)
{

  options->opacity_dynamics      = g_slice_new0 (GimpDynamicOutputOptions);
  options->hardness_dynamics     = g_slice_new0 (GimpDynamicOutputOptions);
  options->rate_dynamics         = g_slice_new0 (GimpDynamicOutputOptions);
  options->size_dynamics         = g_slice_new0 (GimpDynamicOutputOptions);
  options->aspect_ratio_dynamics = g_slice_new0 (GimpDynamicOutputOptions);
  options->color_dynamics        = g_slice_new0 (GimpDynamicOutputOptions);
  options->angle_dynamics        = g_slice_new0 (GimpDynamicOutputOptions);

}


static void
gimp_dynamics_options_finalize (GObject *object)
{
  GimpDynamicsOptions *options = GIMP_DYNAMICS_OPTIONS (object);

  g_slice_free (GimpDynamicOutputOptions,  options->opacity_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->hardness_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->rate_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->size_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->aspect_ratio_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->color_dynamics);
  g_slice_free (GimpDynamicOutputOptions,  options->angle_dynamics);


  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dynamics_options_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpDynamicsOptions *options           = GIMP_DYNAMICS_OPTIONS (object);

  GimpDynamicOutputOptions *opacity_dynamics      = options->opacity_dynamics;
  GimpDynamicOutputOptions *hardness_dynamics     = options->hardness_dynamics;
  GimpDynamicOutputOptions *rate_dynamics         = options->rate_dynamics;
  GimpDynamicOutputOptions *size_dynamics         = options->size_dynamics;
  GimpDynamicOutputOptions *aspect_ratio_dynamics = options->aspect_ratio_dynamics;
  GimpDynamicOutputOptions *color_dynamics        = options->color_dynamics;
  GimpDynamicOutputOptions *angle_dynamics        = options->angle_dynamics;

  switch (property_id)
    {


    case PROP_PRESSURE_OPACITY:
      opacity_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_HARDNESS:
      hardness_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_RATE:
      rate_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_SIZE:
      size_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_ASPECT_RATIO:
      aspect_ratio_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_COLOR:
      color_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_ANGLE:
      angle_dynamics->pressure = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_OPACITY:
      opacity_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_HARDNESS:
      hardness_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_RATE:
      rate_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_SIZE:
      size_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_ASPECT_RATIO:
      aspect_ratio_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_COLOR:
      color_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_VELOCITY_ANGLE:
      angle_dynamics->velocity = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_OPACITY:
      opacity_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_HARDNESS:
      hardness_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_RATE:
      rate_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_SIZE:
      size_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_ASPECT_RATIO:
      aspect_ratio_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_COLOR:
      color_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_DIRECTION_ANGLE:
      angle_dynamics->direction = g_value_get_boolean (value);
      break;

    case PROP_TILT_OPACITY:
      opacity_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_HARDNESS:
      hardness_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_RATE:
      rate_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_SIZE:
      size_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_ASPECT_RATIO:
      aspect_ratio_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_COLOR:
      color_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_TILT_ANGLE:
      angle_dynamics->tilt = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_OPACITY:
      opacity_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_HARDNESS:
      hardness_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_RATE:
      rate_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_SIZE:
      size_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_ASPECT_RATIO:
      aspect_ratio_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_COLOR:
      color_dynamics->random = g_value_get_boolean (value);
      break;

    case PROP_RANDOM_ANGLE:
      angle_dynamics->random = g_value_get_boolean (value);
      break;


/*Fading*/
    case PROP_FADING_OPACITY:
      opacity_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_HARDNESS:
      hardness_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_RATE:
      rate_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_SIZE:
      size_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_ASPECT_RATIO:
      aspect_ratio_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_COLOR:
      color_dynamics->fade = g_value_get_boolean (value);
      break;

    case PROP_FADING_ANGLE:
      angle_dynamics->fade = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
gimp_dynamics_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDynamicsOptions *options           = GIMP_DYNAMICS_OPTIONS (object);
  GimpDynamicOutputOptions *opacity_dynamics      = options->opacity_dynamics;
  GimpDynamicOutputOptions *hardness_dynamics     = options->hardness_dynamics;
  GimpDynamicOutputOptions *rate_dynamics         = options->rate_dynamics;
  GimpDynamicOutputOptions *size_dynamics         = options->size_dynamics;
  GimpDynamicOutputOptions *aspect_ratio_dynamics = options->aspect_ratio_dynamics;
  GimpDynamicOutputOptions *color_dynamics        = options->color_dynamics;
  GimpDynamicOutputOptions *angle_dynamics        = options->angle_dynamics;

  switch (property_id)
    {

    case PROP_PRESSURE_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->pressure);
      break;

    case PROP_PRESSURE_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->pressure);
      break;

    case PROP_PRESSURE_RATE:
      g_value_set_boolean (value, rate_dynamics->pressure);
      break;

    case PROP_PRESSURE_SIZE:
      g_value_set_boolean (value, size_dynamics->pressure);
      break;

    case PROP_PRESSURE_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->pressure);
      break;

    case PROP_PRESSURE_COLOR:
      g_value_set_boolean (value, color_dynamics->pressure);
      break;

    case PROP_PRESSURE_ANGLE:
      g_value_set_boolean (value, angle_dynamics->pressure);
      break;

    case PROP_VELOCITY_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->velocity);
      break;

    case PROP_VELOCITY_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->velocity);
      break;

    case PROP_VELOCITY_RATE:
      g_value_set_boolean (value, rate_dynamics->velocity);
      break;

    case PROP_VELOCITY_SIZE:
      g_value_set_boolean (value, size_dynamics->velocity);
      break;

    case PROP_VELOCITY_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->velocity);
      break;

    case PROP_VELOCITY_COLOR:
      g_value_set_boolean (value, color_dynamics->velocity);
      break;

    case PROP_VELOCITY_ANGLE:
      g_value_set_boolean (value, angle_dynamics->velocity);
      break;

    case PROP_DIRECTION_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->direction);
      break;

    case PROP_DIRECTION_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->direction);
      break;

    case PROP_DIRECTION_RATE:
      g_value_set_boolean (value, rate_dynamics->direction);
      break;

    case PROP_DIRECTION_SIZE:
      g_value_set_boolean (value, size_dynamics->direction);
      break;

    case PROP_DIRECTION_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->direction);
      break;

    case PROP_DIRECTION_COLOR:
      g_value_set_boolean (value, color_dynamics->direction);
      break;

    case PROP_DIRECTION_ANGLE:
      g_value_set_boolean (value, angle_dynamics->direction);
      break;


    case PROP_TILT_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->tilt);
      break;

    case PROP_TILT_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->tilt);
      break;

    case PROP_TILT_RATE:
      g_value_set_boolean (value, rate_dynamics->tilt);
      break;

    case PROP_TILT_SIZE:
      g_value_set_boolean (value, size_dynamics->tilt);
      break;

    case PROP_TILT_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->tilt);
      break;

    case PROP_TILT_COLOR:
      g_value_set_boolean (value, color_dynamics->tilt);
      break;

    case PROP_TILT_ANGLE:
      g_value_set_boolean (value, angle_dynamics->tilt);
      break;

    case PROP_RANDOM_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->random);
      break;

    case PROP_RANDOM_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->random);
      break;

    case PROP_RANDOM_RATE:
      g_value_set_boolean (value, rate_dynamics->random);
      break;

    case PROP_RANDOM_SIZE:
      g_value_set_boolean (value, size_dynamics->random);
      break;

    case PROP_RANDOM_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->random);
      break;

    case PROP_RANDOM_COLOR:
      g_value_set_boolean (value, color_dynamics->random);
      break;

    case PROP_RANDOM_ANGLE:
      g_value_set_boolean (value, angle_dynamics->random);
      break;

/*fading*/

    case PROP_FADING_OPACITY:
      g_value_set_boolean (value, opacity_dynamics->fade);
      break;

    case PROP_FADING_HARDNESS:
      g_value_set_boolean (value, hardness_dynamics->fade);
      break;

    case PROP_FADING_RATE:
      g_value_set_boolean (value, rate_dynamics->fade);
      break;

    case PROP_FADING_SIZE:
      g_value_set_boolean (value, size_dynamics->fade);
      break;

    case PROP_FADING_ASPECT_RATIO:
      g_value_set_boolean (value, aspect_ratio_dynamics->fade);
      break;

    case PROP_FADING_COLOR:
      g_value_set_boolean (value, color_dynamics->fade);
      break;

    case PROP_FADING_ANGLE:
      g_value_set_boolean (value, angle_dynamics->fade);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
gimp_dynamics_options_notify (GObject    *object,
                              GParamSpec *pspec)
{
/*
  GimpDynamicsOptions *options = GIMP_DYNAMICS_OPTIONS (object);

  if (pspec->param_id == PROP_USE_GRADIENT)
    {
      if (options->gradient_options->use_gradient)
        {
          options->application_mode_save = options->application_mode;
          options->application_mode      = GIMP_PAINT_INCREMENTAL;
        }
      else
        {
          options->application_mode = options->application_mode_save;
        }

      g_object_notify (object, "application-mode");
    }

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);
    */

}

GimpData *
gimp_dynamics_options_new (GimpPaintInfo *paint_info)
{
  GimpDynamicsOptions *options;

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  options = g_object_new (GIMP_TYPE_DYNAMICS_OPTIONS,
                          "gimp",       paint_info->gimp,
                          "name",       GIMP_OBJECT (paint_info)->name,
  //                        "paint-info", paint_info,
                          NULL);

  return options;
}


GimpData *
gimp_dynamics_get_standard (void)
{
  static GimpData *standard_dynamics = NULL;

  if (! standard_dynamics)
    {
      standard_dynamics = gimp_dynamics_options_new ("Standard");

      standard_dynamics->dirty = FALSE;
      gimp_data_make_internal (standard_dynamics,
                               "gimp-dynamics-standard");

      g_object_ref (standard_dynamics);
    }

  return standard_dynamics;
}


/* Calculates dynamics mix to be used for same parameter
 * (velocity/pressure/direction/tilt/random) mix Needed in may places and tools.
 *
 * Added one parameter: fading, the 6th driving factor.
 * (velocity/pressure/direction/tilt/random/fading)
 */
