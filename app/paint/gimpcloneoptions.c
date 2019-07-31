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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "gimpcloneoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CLONE_TYPE
};


static void   gimp_clone_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_clone_options_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


G_DEFINE_TYPE (GimpCloneOptions, gimp_clone_options, GIMP_TYPE_SOURCE_OPTIONS)

#define parent_class gimp_clone_options_parent_class


static void
gimp_clone_options_class_init (GimpCloneOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_clone_options_set_property;
  object_class->get_property = gimp_clone_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CLONE_TYPE,
                         "clone-type",
                         _("Source"),
                         NULL,
                         GIMP_TYPE_CLONE_TYPE,
                         GIMP_CLONE_IMAGE,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_clone_options_init (GimpCloneOptions *options)
{
}

static void
gimp_clone_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_TYPE:
      options->clone_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_clone_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_TYPE:
      g_value_set_enum (value, options->clone_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
