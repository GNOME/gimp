/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockcontainer.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "gimpdockcontainer.h"


static void   gimp_dock_container_iface_base_init   (GimpDockContainerInterface *container_iface);


GType
gimp_dock_container_interface_get_type (void)
{
  static GType iface_type = 0;

  if (! iface_type)
    {
      const GTypeInfo iface_info =
      {
        sizeof (GimpDockContainerInterface),
        (GBaseInitFunc)     gimp_dock_container_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpDockContainerInterface",
                                           &iface_info,
                                           0);

      g_type_interface_add_prerequisite (iface_type, GTK_TYPE_WIDGET);
    }

  return iface_type;
}

static void
gimp_dock_container_iface_base_init (GimpDockContainerInterface *container_iface)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

  initialized = TRUE;

  container_iface->get_docks = NULL;
}

GList *
gimp_dock_container_get_docks (GimpDockContainer *container)
{
  g_return_val_if_fail (GIMP_IS_DOCK_CONTAINER (container), NULL);

  return GIMP_DOCK_CONTAINER_GET_INTERFACE (container)->get_docks (container);
}
