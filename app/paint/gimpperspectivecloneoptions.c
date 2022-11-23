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

#include "ligmaperspectivecloneoptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CLONE_MODE
};


static void   ligma_perspective_clone_options_set_property (GObject      *object,
                                                           guint         property_id,
                                                           const GValue *value,
                                                           GParamSpec   *pspec);
static void   ligma_perspective_clone_options_get_property (GObject      *object,
                                                           guint         property_id,
                                                           GValue       *value,
                                                           GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaPerspectiveCloneOptions, ligma_perspective_clone_options,
               LIGMA_TYPE_CLONE_OPTIONS)


static void
ligma_perspective_clone_options_class_init (LigmaPerspectiveCloneOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_perspective_clone_options_set_property;
  object_class->get_property = ligma_perspective_clone_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CLONE_MODE,
                         "clone-mode",
                         NULL, NULL,
                         LIGMA_TYPE_PERSPECTIVE_CLONE_MODE,
                         LIGMA_PERSPECTIVE_CLONE_MODE_ADJUST,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_perspective_clone_options_init (LigmaPerspectiveCloneOptions *options)
{
}

static void
ligma_perspective_clone_options_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  LigmaPerspectiveCloneOptions *options = LIGMA_PERSPECTIVE_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_MODE:
      options->clone_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_perspective_clone_options_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  LigmaPerspectiveCloneOptions *options = LIGMA_PERSPECTIVE_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_MODE:
      g_value_set_enum (value, options->clone_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
