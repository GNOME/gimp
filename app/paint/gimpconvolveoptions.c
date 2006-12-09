/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "gimpconvolveoptions.h"


#define DEFAULT_CONVOLVE_TYPE  GIMP_BLUR_CONVOLVE
#define DEFAULT_CONVOLVE_RATE  50.0


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_RATE
};


static void   gimp_convolve_options_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_convolve_options_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (GimpConvolveOptions, gimp_convolve_options,
               GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_convolve_options_class_init (GimpConvolveOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_convolve_options_set_property;
  object_class->get_property = gimp_convolve_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TYPE,
                                 "type", NULL,
                                 GIMP_TYPE_CONVOLVE_TYPE,
                                 DEFAULT_CONVOLVE_TYPE,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_RATE,
                                   "rate", NULL,
                                   0.0, 100.0, DEFAULT_CONVOLVE_RATE,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_convolve_options_init (GimpConvolveOptions *options)
{
}

static void
gimp_convolve_options_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpConvolveOptions *options = GIMP_CONVOLVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      options->type = g_value_get_enum (value);
      break;
    case PROP_RATE:
      options->rate = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_convolve_options_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpConvolveOptions *options = GIMP_CONVOLVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, options->type);
      break;
    case PROP_RATE:
      g_value_set_double (value, options->rate);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
