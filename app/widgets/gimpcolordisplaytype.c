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

#include "widgets-types.h"

#include "gimpcolordisplaytype.h"


/**
 * GimpColorDisplayType:
 *
 * A helper object to list types/categories of color displays
 */

struct _GimpColorDisplayType
{
  GimpViewable parent_instance;

  GType        gtype;
};

G_DEFINE_TYPE (GimpColorDisplayType, gimp_color_display_type,
               GIMP_TYPE_VIEWABLE)


static void
gimp_color_display_type_init (GimpColorDisplayType *self)
{
}

static void
gimp_color_display_type_class_init (GimpColorDisplayTypeClass *klass)
{
}

GimpColorDisplayType *
gimp_color_display_type_new (GType gtype)
{
  GimpColorDisplayClass *color_display_class;
  GimpColorDisplayType  *type;

  g_return_val_if_fail (g_type_is_a (gtype, GIMP_TYPE_COLOR_DISPLAY), NULL);

  color_display_class = g_type_class_ref (gtype);

  type = g_object_new (GIMP_TYPE_COLOR_DISPLAY_TYPE,
                       "name",      color_display_class->name,
                       "icon-name", color_display_class->icon_name,
                       NULL);

  type->gtype = gtype;

  g_type_class_unref (color_display_class);

  return type;
}

GType
gimp_color_display_type_get_gtype (GimpColorDisplayType *self)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_TYPE (self), G_TYPE_INVALID);

  return self->gtype;
}

GListModel *
gimp_color_display_type_get_model (void)
{
  GListStore *types;
  GType      *color_display_types;
  guint       n_color_display_types;
  gint        i;

  types = g_list_store_new (GIMP_TYPE_COLOR_DISPLAY_TYPE);

  color_display_types = g_type_children (GIMP_TYPE_COLOR_DISPLAY,
                                         &n_color_display_types);

  for (i = 0; i < n_color_display_types; i++)
    {
      GimpColorDisplayType *type;

      type = gimp_color_display_type_new (color_display_types[i]);
      g_list_store_append (G_LIST_STORE (types), type);
      g_object_unref (type);
    }

  g_free (color_display_types);

  g_list_store_sort (types, (GCompareDataFunc) gimp_object_name_collate,
                     NULL);

  return G_LIST_MODEL (types);
}
