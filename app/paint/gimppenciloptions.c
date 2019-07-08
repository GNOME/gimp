/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "gimppenciloptions.h"


#define PENCIL_DEFAULT_HARD TRUE


enum
{
  PROP_0,
  PROP_HARD
};


static void   gimp_pencil_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_pencil_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (GimpPencilOptions, gimp_pencil_options,
               GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_pencil_options_class_init (GimpPencilOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_pencil_options_set_property;
  object_class->get_property = gimp_pencil_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_HARD,
                            "hard",
                            NULL, NULL,
                            PENCIL_DEFAULT_HARD,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_pencil_options_init (GimpPencilOptions *options)
{
}

static void
gimp_pencil_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HARD:
      options->hard = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pencil_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HARD:
      g_value_set_boolean (value, options->hard);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
