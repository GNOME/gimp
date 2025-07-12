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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "gimpcontrollercategory.h"


/**
 * GimpControllerCategory:
 *
 * A helper object to list types/categories of controllers
 */

struct _GimpControllerCategory {
  GObject      parent_instance;

  const gchar *name;
  const gchar *icon_name;
  GType        gtype;
};

G_DEFINE_TYPE (GimpControllerCategory, gimp_controller_category,
               GIMP_TYPE_VIEWABLE)


static void
gimp_controller_category_init (GimpControllerCategory *self)
{
}

static void
gimp_controller_category_class_init (GimpControllerCategoryClass *klass)
{
}

GimpControllerCategory *
gimp_controller_category_new (GType gtype)
{
  GimpControllerClass    *controller_class;
  GimpControllerCategory *category;

  g_return_val_if_fail (g_type_is_a (gtype, GIMP_TYPE_CONTROLLER), NULL);

  controller_class = g_type_class_ref (gtype);

  category = g_object_new (GIMP_TYPE_CONTROLLER_CATEGORY,
                           "name",      controller_class->name,
                           "icon-name", controller_class->icon_name,
                           NULL);

  category->gtype = gtype;

  g_type_class_unref (controller_class);

  return category;
}

GType
gimp_controller_category_get_gtype (GimpControllerCategory *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_CATEGORY (self), G_TYPE_INVALID);

  return self->gtype;
}
