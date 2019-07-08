/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockcontainer.h
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

#ifndef __GIMP_DOCK_CONTAINER_H__
#define __GIMP_DOCK_CONTAINER_H__


#define GIMP_TYPE_DOCK_CONTAINER               (gimp_dock_container_get_type ())
#define GIMP_DOCK_CONTAINER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCK_CONTAINER, GimpDockContainer))
#define GIMP_IS_DOCK_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCK_CONTAINER))
#define GIMP_DOCK_CONTAINER_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_DOCK_CONTAINER, GimpDockContainerInterface))


typedef struct _GimpDockContainerInterface GimpDockContainerInterface;

struct _GimpDockContainerInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GList             * (* get_docks)          (GimpDockContainer   *container);
  GimpDialogFactory * (* get_dialog_factory) (GimpDockContainer   *container);
  GimpUIManager     * (* get_ui_manager)     (GimpDockContainer   *container);
  void                (* add_dock)           (GimpDockContainer   *container,
                                              GimpDock            *dock,
                                              GimpSessionInfoDock *dock_info);
  GimpAlignmentType   (* get_dock_side)      (GimpDockContainer   *container,
                                              GimpDock            *dock);
};


GType               gimp_dock_container_get_type           (void) G_GNUC_CONST;

GList             * gimp_dock_container_get_docks          (GimpDockContainer   *container);
GimpDialogFactory * gimp_dock_container_get_dialog_factory (GimpDockContainer   *container);
GimpUIManager     * gimp_dock_container_get_ui_manager     (GimpDockContainer   *container);
void                gimp_dock_container_add_dock           (GimpDockContainer   *container,
                                                            GimpDock            *dock,
                                                            GimpSessionInfoDock *dock_info);
GimpAlignmentType   gimp_dock_container_get_dock_side      (GimpDockContainer   *container,
                                                            GimpDock            *dock);


#endif  /*  __GIMP_DOCK_CONTAINER_H__  */
