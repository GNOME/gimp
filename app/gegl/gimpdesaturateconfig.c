/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdesaturateconfig.c
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"

#include "gimp-gegl-types.h"

#include "gimpdesaturateconfig.h"


enum
{
  PROP_0,
  PROP_MODE
};


static void   gimp_desaturate_config_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void   gimp_desaturate_config_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpDesaturateConfig, gimp_desaturate_config,
                         GIMP_TYPE_IMAGE_MAP_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_desaturate_config_parent_class


static void
gimp_desaturate_config_class_init (GimpDesaturateConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property       = gimp_desaturate_config_set_property;
  object_class->get_property       = gimp_desaturate_config_get_property;

  /*FIXME: change string when a desaturate icon gets added */
  viewable_class->default_stock_id = "gimp-convert-grayscale";

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_MODE,
                                 "mode",
                                 "Desaturate mode",
                                 GIMP_TYPE_DESATURATE_MODE,
                                 GIMP_DESATURATE_LIGHTNESS, 0);
}

static void
gimp_desaturate_config_init (GimpDesaturateConfig *self)
{
}

static void
gimp_desaturate_config_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpDesaturateConfig *self = GIMP_DESATURATE_CONFIG (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, self->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_desaturate_config_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpDesaturateConfig *self = GIMP_DESATURATE_CONFIG (object);

  switch (property_id)
    {
    case PROP_MODE:
      self->mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
