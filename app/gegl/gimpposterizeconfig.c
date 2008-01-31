/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpposterizeconfig.c
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

#include "gimpposterizeconfig.h"


enum
{
  PROP_0,
  PROP_LEVELS
};


static void   gimp_posterize_config_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void   gimp_posterize_config_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpPosterizeConfig, gimp_posterize_config,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_posterize_config_parent_class


static void
gimp_posterize_config_class_init (GimpPosterizeConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_posterize_config_set_property;
  object_class->get_property = gimp_posterize_config_get_property;

  g_object_class_install_property (object_class, PROP_LEVELS,
                                   g_param_spec_int ("levels",
                                                     "Levels",
                                                     "Posterize levels",
                                                     2, 256, 3,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_posterize_config_init (GimpPosterizeConfig *self)
{
}

static void
gimp_posterize_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpPosterizeConfig *self = GIMP_POSTERIZE_CONFIG (object);

  switch (property_id)
    {
    case PROP_LEVELS:
      g_value_set_int (value, self->levels);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_posterize_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpPosterizeConfig *self = GIMP_POSTERIZE_CONFIG (object);

  switch (property_id)
    {
    case PROP_LEVELS:
      self->levels = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
