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

#include "gimpcontrollertype.h"


/**
 * GimpControllerType:
 *
 * A helper object to list types/categories of controllers
 */

struct _GimpControllerType
{
  GimpViewable parent_instance;

  GType        gtype;
};

G_DEFINE_TYPE (GimpControllerType, gimp_controller_type,
               GIMP_TYPE_VIEWABLE)


static void
gimp_controller_type_init (GimpControllerType *self)
{
}

static void
gimp_controller_type_class_init (GimpControllerTypeClass *klass)
{
}

GimpControllerType *
gimp_controller_type_new (GType gtype)
{
  GimpControllerClass *controller_class;
  GimpControllerType  *type;

  g_return_val_if_fail (g_type_is_a (gtype, GIMP_TYPE_CONTROLLER), NULL);

  controller_class = g_type_class_ref (gtype);

  type = g_object_new (GIMP_TYPE_CONTROLLER_TYPE,
                       "name",      controller_class->name,
                       "icon-name", controller_class->icon_name,
                       NULL);

  type->gtype = gtype;

  g_type_class_unref (controller_class);

  return type;
}

GType
gimp_controller_type_get_gtype (GimpControllerType *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_TYPE (self), G_TYPE_INVALID);

  return self->gtype;
}

GListModel *
gimp_controller_type_get_model (void)
{
  GListStore *types;
  GType      *controller_types;
  guint       n_controller_types;
  gint        i;

  types = g_list_store_new (GIMP_TYPE_CONTROLLER_TYPE);

  controller_types = g_type_children (GIMP_TYPE_CONTROLLER,
                                      &n_controller_types);

  for (i = 0; i < n_controller_types; i++)
    {
      GimpControllerType *type;

      type = gimp_controller_type_new (controller_types[i]);
      g_list_store_append (G_LIST_STORE (types), type);
      g_object_unref (type);
    }

  g_free (controller_types);

  return G_LIST_MODEL (types);
}
