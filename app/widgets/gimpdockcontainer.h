/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockcontainer.h
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

#ifndef __LIGMA_DOCK_CONTAINER_H__
#define __LIGMA_DOCK_CONTAINER_H__


#define LIGMA_TYPE_DOCK_CONTAINER (ligma_dock_container_get_type ())
G_DECLARE_INTERFACE (LigmaDockContainer, ligma_dock_container, LIGMA, DOCK_CONTAINER, GtkWidget)


struct _LigmaDockContainerInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GList             * (* get_docks)          (LigmaDockContainer   *container);
  LigmaDialogFactory * (* get_dialog_factory) (LigmaDockContainer   *container);
  LigmaUIManager     * (* get_ui_manager)     (LigmaDockContainer   *container);
  void                (* add_dock)           (LigmaDockContainer   *container,
                                              LigmaDock            *dock,
                                              LigmaSessionInfoDock *dock_info);
  LigmaAlignmentType   (* get_dock_side)      (LigmaDockContainer   *container,
                                              LigmaDock            *dock);
};


GList             * ligma_dock_container_get_docks          (LigmaDockContainer   *container);
LigmaDialogFactory * ligma_dock_container_get_dialog_factory (LigmaDockContainer   *container);
LigmaUIManager     * ligma_dock_container_get_ui_manager     (LigmaDockContainer   *container);
void                ligma_dock_container_add_dock           (LigmaDockContainer   *container,
                                                            LigmaDock            *dock,
                                                            LigmaSessionInfoDock *dock_info);
LigmaAlignmentType   ligma_dock_container_get_dock_side      (LigmaDockContainer   *container,
                                                            LigmaDock            *dock);


#endif  /*  __LIGMA_DOCK_CONTAINER_H__  */
