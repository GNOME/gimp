/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "paint-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimppaintoptions.h"


#define DEFAULT_APPLICATION_MODE  GIMP_PAINT_CONSTANT

#define DEFAULT_PRESSURE_OPACITY  TRUE
#define DEFAULT_PRESSURE_PRESSURE TRUE
#define DEFAULT_PRESSURE_RATE     FALSE
#define DEFAULT_PRESSURE_SIZE     FALSE
#define DEFAULT_PRESSURE_COLOR    FALSE

#define DEFAULT_USE_FADE          FALSE
#define DEFAULT_FADE_LENGTH       100.0
#define DEFAULT_FADE_UNIT         GIMP_UNIT_PIXEL
#define DEFAULT_USE_GRADIENT      FALSE
#define DEFAULT_GRADIENT_LENGTH   100.0
#define DEFAULT_GRADIENT_UNIT     GIMP_UNIT_PIXEL
#define DEFAULT_GRADIENT_TYPE     GIMP_GRADIENT_LOOP_TRIANGLE


enum
{
  PROP_0,
  PROP_APPLICATION_MODE,
  PROP_PRESSURE_OPACITY,
  PROP_PRESSURE_PRESSURE,
  PROP_PRESSURE_RATE,
  PROP_PRESSURE_SIZE,
  PROP_PRESSURE_COLOR,
  PROP_USE_FADE,
  PROP_FADE_LENGTH,
  PROP_FADE_UNIT,
  PROP_USE_GRADIENT,
  PROP_GRADIENT_LENGTH,
  PROP_GRADIENT_UNIT,
  PROP_GRADIENT_TYPE
};


static void   gimp_paint_options_init       (GimpPaintOptions      *options);
static void   gimp_paint_options_class_init (GimpPaintOptionsClass *options_class);

static void   gimp_paint_options_set_property (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   gimp_paint_options_get_property (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);
static void   gimp_paint_options_notify       (GObject         *object,
                                               GParamSpec      *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_paint_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaintOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paint_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paint_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpPaintOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_paint_options_class_init (GimpPaintOptionsClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_paint_options_set_property;
  object_class->get_property = gimp_paint_options_get_property;
  object_class->notify       = gimp_paint_options_notify;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_APPLICATION_MODE,
                                 "application-mode", NULL,
                                 GIMP_TYPE_PAINT_APPLICATION_MODE,
                                 DEFAULT_APPLICATION_MODE,
                                 0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_OPACITY,
                                    "pressure-opacity", NULL,
                                    DEFAULT_PRESSURE_OPACITY,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_PRESSURE,
                                    "pressure-pressure", NULL,
                                    DEFAULT_PRESSURE_PRESSURE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_RATE,
                                    "pressure-rate", NULL,
                                    DEFAULT_PRESSURE_RATE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_SIZE,
                                    "pressure-size", NULL,
                                    DEFAULT_PRESSURE_SIZE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PRESSURE_COLOR,
                                    "pressure-color", NULL,
                                    DEFAULT_PRESSURE_COLOR,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_FADE,
                                    "use-fade", NULL,
                                    DEFAULT_USE_FADE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FADE_LENGTH,
                                   "fade-length", NULL,
                                   0.0, 32767.0, DEFAULT_FADE_LENGTH,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_FADE_UNIT,
                                 "fade-unit", NULL,
                                 TRUE, DEFAULT_FADE_UNIT,
                                 0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_GRADIENT,
                                    "use-gradient", NULL,
                                    DEFAULT_USE_GRADIENT, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_GRADIENT_LENGTH,
                                   "gradient-length", NULL,
                                   0.0, 32767.0, DEFAULT_GRADIENT_LENGTH,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_GRADIENT_UNIT,
                                 "gradient-unit", NULL,
                                 TRUE, DEFAULT_GRADIENT_UNIT,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRADIENT_TYPE,
                                 "gradient-type", NULL,
                                 GIMP_TYPE_GRADIENT_PAINT_MODE,
                                 DEFAULT_GRADIENT_TYPE,
                                 0);
}

static void
gimp_paint_options_init (GimpPaintOptions *options)
{
  options->application_mode_save = DEFAULT_APPLICATION_MODE;

  options->pressure_options = g_new0 (GimpPressureOptions, 1);
  options->gradient_options = g_new0 (GimpGradientOptions, 1);
}

static void
gimp_paint_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpPaintOptions    *options;
  GimpPressureOptions *pressure_options;
  GimpGradientOptions *gradient_options;

  options = GIMP_PAINT_OPTIONS (object);

  pressure_options = options->pressure_options;
  gradient_options = options->gradient_options;

  switch (property_id)
    {
    case PROP_APPLICATION_MODE:
      options->application_mode = g_value_get_enum (value);
      break;

    case PROP_PRESSURE_OPACITY:
      pressure_options->opacity = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_PRESSURE:
      pressure_options->pressure = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_RATE:
      pressure_options->rate = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_SIZE:
      pressure_options->size = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE_COLOR:
      pressure_options->color = g_value_get_boolean (value);
      break;

    case PROP_USE_FADE:
      gradient_options->use_fade = g_value_get_boolean (value);
      break;
    case PROP_FADE_LENGTH:
      gradient_options->fade_length = g_value_get_double (value);
      break;
    case PROP_FADE_UNIT:
      gradient_options->fade_unit = g_value_get_int (value);
      break;

    case PROP_USE_GRADIENT:
      gradient_options->use_gradient = g_value_get_boolean (value);
      break;
    case PROP_GRADIENT_LENGTH:
      gradient_options->gradient_length = g_value_get_double (value);
      break;
    case PROP_GRADIENT_UNIT:
      gradient_options->gradient_unit = g_value_get_int (value);
      break;
    case PROP_GRADIENT_TYPE:
      gradient_options->gradient_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPaintOptions    *options;
  GimpPressureOptions *pressure_options;
  GimpGradientOptions *gradient_options;

  options = GIMP_PAINT_OPTIONS (object);

  pressure_options = options->pressure_options;
  gradient_options = options->gradient_options;

  switch (property_id)
    {
    case PROP_APPLICATION_MODE:
      g_value_set_enum (value, options->application_mode);
      break;

    case PROP_PRESSURE_OPACITY:
      g_value_set_boolean (value, pressure_options->opacity);
      break;
    case PROP_PRESSURE_PRESSURE:
      g_value_set_boolean (value, pressure_options->pressure);
      break;
    case PROP_PRESSURE_RATE:
      g_value_set_boolean (value, pressure_options->rate);
      break;
    case PROP_PRESSURE_SIZE:
      g_value_set_boolean (value, pressure_options->size);
      break;
    case PROP_PRESSURE_COLOR:
      g_value_set_boolean (value, pressure_options->color);
      break;

    case PROP_USE_FADE:
      g_value_set_boolean (value, gradient_options->use_fade);
      break;
    case PROP_FADE_LENGTH:
      g_value_set_double (value, gradient_options->fade_length);
      break;
    case PROP_FADE_UNIT:
      g_value_set_int (value, gradient_options->fade_unit);
      break;

    case PROP_USE_GRADIENT:
      g_value_set_boolean (value, gradient_options->use_gradient);
      break;
    case PROP_GRADIENT_LENGTH:
      g_value_set_double (value, gradient_options->gradient_length);
      break;
    case PROP_GRADIENT_UNIT:
      g_value_set_int (value, gradient_options->gradient_unit);
      break;
    case PROP_GRADIENT_TYPE:
      g_value_set_enum (value, gradient_options->gradient_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_options_notify (GObject    *object,
                           GParamSpec *pspec)
{
  GimpPaintOptions *options;

  options = GIMP_PAINT_OPTIONS (object);

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
}

GimpPaintOptions *
gimp_paint_options_new (Gimp  *gimp,
                        GType  options_type)
{
  GimpPaintOptions *options;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (options_type, GIMP_TYPE_PAINT_OPTIONS),
                        NULL);

  options = g_object_new (options_type,
                          "gimp", gimp,
                          NULL);

  return options;
}
