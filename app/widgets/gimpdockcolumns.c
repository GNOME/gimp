/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockcolumns.c
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpdock.h"
#include "gimpdockcolumns.h"


struct _GimpDockColumnsPrivate
{
  int dummy;
};


G_DEFINE_TYPE (GimpDockColumns, gimp_dock_columns, GTK_TYPE_HBOX)

#define parent_class gimp_dock_columns_parent_class


static void
gimp_dock_columns_class_init (GimpDockColumnsClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpDockColumnsPrivate));
}

static void
gimp_dock_columns_init (GimpDockColumns *dock_columns)
{
  dock_columns->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_columns,
                                                GIMP_TYPE_DOCK_COLUMNS,
                                                GimpDockColumnsPrivate);
}

void
gimp_dock_columns_add_dock (GimpDockColumns *dock_columns,
                            GimpDock        *dock)
{
  g_return_if_fail (GIMP_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (GIMP_IS_DOCK (dock));

  gtk_box_pack_start (GTK_BOX (dock_columns), GTK_WIDGET (dock),
                      TRUE, TRUE, 0);
}
