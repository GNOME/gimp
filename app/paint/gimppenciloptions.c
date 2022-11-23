/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "paint-types.h"

#include "ligmapenciloptions.h"


#define PENCIL_DEFAULT_HARD TRUE


enum
{
  PROP_0,
  PROP_HARD
};


static void   ligma_pencil_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   ligma_pencil_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaPencilOptions, ligma_pencil_options,
               LIGMA_TYPE_PAINT_OPTIONS)


static void
ligma_pencil_options_class_init (LigmaPencilOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_pencil_options_set_property;
  object_class->get_property = ligma_pencil_options_get_property;

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_HARD,
                            "hard",
                            NULL, NULL,
                            PENCIL_DEFAULT_HARD,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_pencil_options_init (LigmaPencilOptions *options)
{
}

static void
ligma_pencil_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaPaintOptions *options = LIGMA_PAINT_OPTIONS (object);

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
ligma_pencil_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaPaintOptions *options = LIGMA_PAINT_OPTIONS (object);

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
