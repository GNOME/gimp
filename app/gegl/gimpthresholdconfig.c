/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpthresholdconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "gegl-types.h"

/*  temp cruft  */
#include "base/threshold.h"

#include "gimpthresholdconfig.h"


enum
{
  PROP_0,
  PROP_LOW,
  PROP_HIGH
};


static void   gimp_threshold_config_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void   gimp_threshold_config_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpThresholdConfig, gimp_threshold_config,
                         GIMP_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_threshold_config_parent_class


static void
gimp_threshold_config_class_init (GimpThresholdConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property       = gimp_threshold_config_set_property;
  object_class->get_property       = gimp_threshold_config_get_property;

  viewable_class->default_stock_id = "gimp-tool-threshold";

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LOW,
                                   "low",
                                   "Low threshold",
                                   0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HIGH,
                                   "high",
                                   "High threshold",
                                   0.0, 1.0, 1.0, 0);
}

static void
gimp_threshold_config_init (GimpThresholdConfig *self)
{
}

static void
gimp_threshold_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpThresholdConfig *self = GIMP_THRESHOLD_CONFIG (object);

  switch (property_id)
    {
    case PROP_LOW:
      g_value_set_double (value, self->low);
      break;

    case PROP_HIGH:
      g_value_set_double (value, self->high);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_threshold_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpThresholdConfig *self = GIMP_THRESHOLD_CONFIG (object);

  switch (property_id)
    {
    case PROP_LOW:
      self->low = g_value_get_double (value);
      break;

    case PROP_HIGH:
      self->high = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  temp cruft  */

void
gimp_threshold_config_to_cruft (GimpThresholdConfig *config,
                                Threshold           *cruft)
{
  g_return_if_fail (GIMP_IS_THRESHOLD_CONFIG (config));
  g_return_if_fail (cruft != NULL);

  cruft->low_threshold  = config->low  * 255.999;
  cruft->high_threshold = config->high * 255.999;
}
