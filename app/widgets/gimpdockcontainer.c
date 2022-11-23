/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockcontainer.c
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

#include "ligmadockcontainer.h"


G_DEFINE_INTERFACE (LigmaDockContainer, ligma_dock_container, GTK_TYPE_WIDGET)


/*  private functions  */


static void
ligma_dock_container_default_init (LigmaDockContainerInterface *iface)
{
}


/*  public functions  */


/**
 * ligma_dock_container_get_docks:
 * @container: A #LigmaDockContainer
 *
 * Returns: A list of #LigmaDock:s in the dock container. Free with
 *          g_list_free() when done.
 **/
GList *
ligma_dock_container_get_docks (LigmaDockContainer *container)
{
  LigmaDockContainerInterface *iface;

  g_return_val_if_fail (LIGMA_IS_DOCK_CONTAINER (container), NULL);

  iface = LIGMA_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_docks)
    return iface->get_docks (container);

  return NULL;
}

/**
 * ligma_dock_container_get_dialog_factory:
 * @container: A #LigmaDockContainer
 *
 * Returns: The #LigmaDialogFactory of the #LigmaDockContainer
 **/
LigmaDialogFactory *
ligma_dock_container_get_dialog_factory (LigmaDockContainer *container)
{
  LigmaDockContainerInterface *iface;

  g_return_val_if_fail (LIGMA_IS_DOCK_CONTAINER (container), NULL);

  iface = LIGMA_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dialog_factory)
    return iface->get_dialog_factory (container);

  return NULL;
}

/**
 * ligma_dock_container_get_ui_manager:
 * @container: A #LigmaDockContainer
 *
 * Returns: The #LigmaUIManager of the #LigmaDockContainer
 **/
LigmaUIManager *
ligma_dock_container_get_ui_manager (LigmaDockContainer *container)
{
  LigmaDockContainerInterface *iface;

  g_return_val_if_fail (LIGMA_IS_DOCK_CONTAINER (container), NULL);

  iface = LIGMA_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_ui_manager)
    return iface->get_ui_manager (container);

  return NULL;
}

/**
 * ligma_dock_container_add_dock:
 * @container: A #LigmaDockContainer
 * @dock:      The newly created #LigmaDock to add to the container.
 * @dock_info: The #LigmaSessionInfoDock the @dock was created from.
 *
 * Add @dock that was created from @dock_info to @container.
 **/
void
ligma_dock_container_add_dock (LigmaDockContainer   *container,
                              LigmaDock            *dock,
                              LigmaSessionInfoDock *dock_info)
{
  LigmaDockContainerInterface *iface;

  g_return_if_fail (LIGMA_IS_DOCK_CONTAINER (container));

  iface = LIGMA_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->add_dock)
    iface->add_dock (container,
                     dock,
                     dock_info);
}

/**
 * ligma_dock_container_get_dock_side:
 * @container: A #LigmaDockContainer
 * @dock:      A #LigmaDock
 *
 * Returns: What side @dock is in in @container, either
 *          LIGMA_ALIGN_LEFT or LIGMA_ALIGN_RIGHT, or -1 if the side
 *          concept is not applicable.
 **/
LigmaAlignmentType
ligma_dock_container_get_dock_side (LigmaDockContainer   *container,
                                   LigmaDock            *dock)
{
  LigmaDockContainerInterface *iface;

  g_return_val_if_fail (LIGMA_IS_DOCK_CONTAINER (container), -1);

  iface = LIGMA_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dock_side)
    return iface->get_dock_side (container, dock);

  return -1;
}
