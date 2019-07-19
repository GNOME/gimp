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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpdockcontainer.h"


G_DEFINE_INTERFACE (GimpDockContainer, gimp_dock_container, GTK_TYPE_WIDGET)


/*  private functions  */


static void
gimp_dock_container_default_init (GimpDockContainerInterface *iface)
{
}


/*  public functions  */


/**
 * gimp_dock_container_get_docks:
 * @container: A #GimpDockContainer
 *
 * Returns: A list of #GimpDock:s in the dock container. Free with
 *          g_list_free() when done.
 **/
GList *
gimp_dock_container_get_docks (GimpDockContainer *container)
{
  GimpDockContainerInterface *iface;

  g_return_val_if_fail (GIMP_IS_DOCK_CONTAINER (container), NULL);

  iface = GIMP_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_docks)
    return iface->get_docks (container);

  return NULL;
}

/**
 * gimp_dock_container_get_dialog_factory:
 * @container: A #GimpDockContainer
 *
 * Returns: The #GimpDialogFactory of the #GimpDockContainer
 **/
GimpDialogFactory *
gimp_dock_container_get_dialog_factory (GimpDockContainer *container)
{
  GimpDockContainerInterface *iface;

  g_return_val_if_fail (GIMP_IS_DOCK_CONTAINER (container), NULL);

  iface = GIMP_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dialog_factory)
    return iface->get_dialog_factory (container);

  return NULL;
}

/**
 * gimp_dock_container_get_ui_manager:
 * @container: A #GimpDockContainer
 *
 * Returns: The #GimpUIManager of the #GimpDockContainer
 **/
GimpUIManager *
gimp_dock_container_get_ui_manager (GimpDockContainer *container)
{
  GimpDockContainerInterface *iface;

  g_return_val_if_fail (GIMP_IS_DOCK_CONTAINER (container), NULL);

  iface = GIMP_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_ui_manager)
    return iface->get_ui_manager (container);

  return NULL;
}

/**
 * gimp_dock_container_add_dock:
 * @container: A #GimpDockContainer
 * @dock:      The newly created #GimpDock to add to the container.
 * @dock_info: The #GimpSessionInfoDock the @dock was created from.
 *
 * Add @dock that was created from @dock_info to @container.
 **/
void
gimp_dock_container_add_dock (GimpDockContainer   *container,
                              GimpDock            *dock,
                              GimpSessionInfoDock *dock_info)
{
  GimpDockContainerInterface *iface;

  g_return_if_fail (GIMP_IS_DOCK_CONTAINER (container));

  iface = GIMP_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->add_dock)
    iface->add_dock (container,
                     dock,
                     dock_info);
}

/**
 * gimp_dock_container_get_dock_side:
 * @container: A #GimpDockContainer
 * @dock:      A #GimpDock
 *
 * Returns: What side @dock is in in @container, either
 *          GIMP_ALIGN_LEFT or GIMP_ALIGN_RIGHT, or -1 if the side
 *          concept is not applicable.
 **/
GimpAlignmentType
gimp_dock_container_get_dock_side (GimpDockContainer   *container,
                                   GimpDock            *dock)
{
  GimpDockContainerInterface *iface;

  g_return_val_if_fail (GIMP_IS_DOCK_CONTAINER (container), -1);

  iface = GIMP_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dock_side)
    return iface->get_dock_side (container, dock);

  return -1;
}
