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

#include "paint/gimppaintoptions.h"


#include "gimpcurve.h"
#include "gimpdynamicsoutput.h"

#include "gimp-intl.h"


#define DEFAULT_USE_PRESSURE  FALSE
#define DEFAULT_USE_VELOCITY  FALSE
#define DEFAULT_USE_DIRECTION FALSE
#define DEFAULT_USE_TILT      FALSE
#define DEFAULT_USE_RANDOM    FALSE
#define DEFAULT_USE_FADE      FALSE


enum
{
  PROP_0,

  PROP_TYPE,
  PROP_USE_PRESSURE,
  PROP_USE_VELOCITY,
  PROP_USE_DIRECTION,
  PROP_USE_TILT,
  PROP_USE_RANDOM,
  PROP_USE_FADE
};


static void   gimp_dynamics_output_finalize     (GObject      *object);
static void   gimp_dynamics_output_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_dynamics_output_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpDynamicsOutput, gimp_dynamics_output,
                         GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))


#define parent_class gimp_dynamics_output_parent_class


static void
gimp_dynamics_output_class_init (GimpDynamicsOutputClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_dynamics_output_finalize;
  object_class->set_property = gimp_dynamics_output_set_property;
  object_class->get_property = gimp_dynamics_output_get_property;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL,
                                                      _("Output type"),
                                                      GIMP_TYPE_DYNAMICS_OUTPUT_TYPE,
                                                      GIMP_DYNAMICS_OUTPUT_OPACITY,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_PRESSURE,
                                    "use-pressure", NULL,
                                    DEFAULT_USE_PRESSURE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_VELOCITY,
                                    "use-velocity", NULL,
                                    DEFAULT_USE_VELOCITY,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_DIRECTION,
                                    "use-direction", NULL,
                                    DEFAULT_USE_DIRECTION,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_TILT,
                                    "use-tilt", NULL,
                                    DEFAULT_USE_TILT,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_RANDOM,
                                    "use-random", NULL,
                                    DEFAULT_USE_RANDOM,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_FADE,
                                    "use-fade", NULL,
                                    DEFAULT_USE_FADE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_dynamics_output_init (GimpDynamicsOutput *output)
{
  output->pressure_curve  = GIMP_CURVE (gimp_curve_new ("Pressure curve"));
  output->velocity_curve  = GIMP_CURVE (gimp_curve_new ("Velocity curve"));
  output->direction_curve = GIMP_CURVE (gimp_curve_new ("Direction curve"));
  output->tilt_curve      = GIMP_CURVE (gimp_curve_new ("Tilt curve"));
  output->random_curve    = GIMP_CURVE (gimp_curve_new ("Random curve"));
  output->fade_curve      = GIMP_CURVE (gimp_curve_new ("Fade curve"));
}

static void
gimp_dynamics_output_finalize (GObject *object)
{
  GimpDynamicsOutput *output = GIMP_DYNAMICS_OUTPUT (object);

  g_object_unref (output->pressure_curve);
  g_object_unref (output->velocity_curve);
  g_object_unref (output->direction_curve);
  g_object_unref (output->tilt_curve);
  g_object_unref (output->random_curve);
  g_object_unref (output->fade_curve);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dynamics_output_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpDynamicsOutput *output = GIMP_DYNAMICS_OUTPUT (object);

  switch (property_id)
    {
    case PROP_TYPE:
      output->type = g_value_get_enum (value);
      break;

    case PROP_USE_PRESSURE:
      output->use_pressure = g_value_get_boolean (value);
      break;

    case PROP_USE_VELOCITY:
      output->use_velocity = g_value_get_boolean (value);
      break;

    case PROP_USE_DIRECTION:
      output->use_direction = g_value_get_boolean (value);
      break;

    case PROP_USE_TILT:
      output->use_tilt = g_value_get_boolean (value);
      break;

    case PROP_USE_RANDOM:
      output->use_random = g_value_get_boolean (value);
      break;

    case PROP_USE_FADE:
      output->use_fade = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dynamics_output_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpDynamicsOutput *output = GIMP_DYNAMICS_OUTPUT (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, output->type);
      break;

    case PROP_USE_PRESSURE:
      g_value_set_boolean (value, output->use_pressure);
      break;

    case PROP_USE_VELOCITY:
      g_value_set_boolean (value, output->use_velocity);
      break;

    case PROP_USE_DIRECTION:
      g_value_set_boolean (value, output->use_direction);
      break;

    case PROP_USE_TILT:
      g_value_set_boolean (value, output->use_tilt);
      break;

    case PROP_USE_RANDOM:
      g_value_set_boolean (value, output->use_random);
      break;

    case PROP_USE_FADE:
      g_value_set_boolean (value, output->use_fade);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GimpDynamicsOutput *
gimp_dynamics_output_new (const gchar            *name,
                          GimpDynamicsOutputType  type)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_DYNAMICS_OUTPUT,
                       "name", name,
                       "type", type,
                       NULL);
}

gboolean
gimp_dynamics_output_is_enabled (GimpDynamicsOutput *output)
{
  return (output->use_pressure  ||
          output->use_velocity  ||
          output->use_direction ||
          output->use_tilt      ||
          output->use_random    ||
          output->use_fade);
}

gdouble
gimp_dynamics_output_get_linear_value (GimpDynamicsOutput *output,
                                       const GimpCoords   *coords,
                                       GimpPaintOptions   *options,
                                       gdouble             fade_point)
{
  gdouble total   = 0.0;
  gdouble result  = 1.0;
  gint    factors = 0;

  if (output->use_pressure)
    {
      total += coords->pressure;
      factors++;
    }

  if (output->use_velocity)
    {
      total += (1.0 - coords->velocity);
      factors++;
    }

  if (output->use_direction)
    {
      total += fmod (coords->direction + 0.5, 1) ;
      factors++;
    }

  if (output->use_tilt)
    {
      total += 1.0 - sqrt (SQR (coords->xtilt) + SQR (coords->ytilt));
      factors++;
    }

  if (output->use_random)
    {
      total += g_random_double_range (0.0, 1.0);
      factors++;
    }

  if ((output->use_fade) && (options))
    {
      GimpFadeOptions *fade_options = options->fade_options;

      if (fade_options->use_fade)
        {
          total += fade_point;
          factors++;
        }
    }

  if (factors > 0)
    result = total / factors;

#if 0
  g_printerr ("Dynamics queried(linear). Result: %f, factors: %d, total: %f\n",
              result, factors, total);
#endif

  return result;
}

gdouble
gimp_dynamics_output_get_angular_value (GimpDynamicsOutput *output,
                                        const GimpCoords   *coords,
                                        GimpPaintOptions   *options,
                                        gdouble             fade_point)
{
  gdouble total   = 0.0;
  gdouble result  = 0.0; /* angles are additive, so we retun zero for no change. */
  gint    factors = 0;

  if (output->use_pressure)
    {
      total += coords->pressure;
      factors++;
    }

  if (output->use_velocity)
    {
      total += (1.0 - coords->velocity);
      factors++;
    }

  if (output->use_direction)
    {
      total += coords->direction;
      factors++;
    }

  /* For tilt to make sense, it needs to be converted to an angle, not
   * just a vector
   */
  if (output->use_tilt)
    {
      gdouble tilt_x = coords->xtilt;
      gdouble tilt_y = coords->ytilt;
      gdouble tilt   = 0.0;

      if (tilt_x == 0.0)
        {
          if (tilt_y >= 0.0)
            tilt = 0.5;
          else if (tilt_y < 0.0)
            tilt = 0.0;
          else
            tilt = -1.0;
        }
      else
        {
          tilt = atan ((- 1.0 * tilt_y) /
                                tilt_x) / (2 * G_PI);

          if (tilt_x > 0.0)
            tilt = tilt + 0.5;
        }

      tilt = tilt + 0.5; /* correct the angle, its wrong by 180 degrees */

      while (tilt > 1.0)
        tilt -= 1.0;

      while (tilt < 0.0)
        tilt += 1.0;

      total += tilt;
      factors++;
    }

  if (output->use_random)
    {
      total += g_random_double_range (0.0, 1.0);
      factors++;
    }

  if ((output->use_fade) && (options))
    {
      GimpFadeOptions *fade_options = options->fade_options;

      if (fade_options->use_fade)
        {
          total += fade_point;
          factors++;
        }
    }

  if (factors > 0)
    result = total / factors;

#if 0
  g_printerr ("Dynamics queried(angle). Result: %f, factors: %d, total: %f\n",
              result, factors, total);
#endif

   return result;
}

gdouble
gimp_dynamics_output_get_aspect_value (GimpDynamicsOutput *output,
                                       const GimpCoords   *coords,
                                       GimpPaintOptions   *options,
                                       gdouble             fade_point)
{
  gdouble total   = 0.0;
  gdouble result  = 1.0;
  gint    factors = 0;

  if (output->use_pressure)
    {
      total += 2 * coords->pressure;
      factors++;
    }

  if (output->use_velocity)
    {
      total += 2 * coords->velocity;
      factors++;
    }

  if (output->use_direction)
    {
      gdouble direction = 0.0;

      direction = fmod (1 + coords->direction, 0.5) / 0.25;

      if ((coords->direction > 0.0) && (coords->direction < 0.5))
        direction = 1 / direction;

      total += direction;
      factors++;
    }

  if (output->use_tilt)
    {
      total += sqrt ((1 - fabs (coords->xtilt)) / (1 - fabs (coords->ytilt)));
      factors++;
    }

  if (output->use_random)
    {
      gdouble random = g_random_double_range (0.0, 1.0);

      if (random <= 0.5)
        random = 1 / (random / 0.5 * (2.0 - 1.0) + 1.0);
      else
        random = (random - 0.5) / (1.0 - 0.5) * (2.0 - 1.0) + 1.0;

      total += random;
      factors++;
    }

  if ((output->use_fade) && (options))
    {
      GimpFadeOptions *fade_options = options->fade_options;

      if (fade_options->use_fade)
        {
          total += fade_point;
          factors++;
        }
    }

  if (factors > 0)
    result = total / factors;

#if 0
  g_printerr ("Dynamics queried(aspect). Result: %f, factors: %d, total: %f\n",
              result, factors, total);
#endif

  return result;
}
