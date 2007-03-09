/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview-dnd.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONTAINER_TREE_VIEW_DND_H__
#define __GIMP_CONTAINER_TREE_VIEW_DND_H__


void     gimp_container_tree_view_drag_leave  (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               guint                  time,
                                               GimpContainerTreeView *view);
gboolean gimp_container_tree_view_drag_motion (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               guint                  time,
                                               GimpContainerTreeView *view);
gboolean gimp_container_tree_view_drag_drop   (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               guint                  time,
                                               GimpContainerTreeView *view);
void     gimp_container_tree_view_drag_data_received
                                              (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               GtkSelectionData      *selection_data,
                                               guint                  info,
                                               guint                  time,
                                               GimpContainerTreeView *view);

gboolean
gimp_container_tree_view_real_drop_possible (GimpContainerTreeView   *tree_view,
                                             GimpDndType              src_type,
                                             GimpViewable            *src_viewable,
                                             GimpViewable            *dest_viewable,
                                             GtkTreeViewDropPosition  drop_pos,
                                             GtkTreeViewDropPosition *return_drop_pos,
                                             GdkDragAction           *return_drag_action);
void
gimp_container_tree_view_real_drop_viewable (GimpContainerTreeView   *tree_view,
                                             GimpViewable            *src_viewable,
                                             GimpViewable            *dest_viewable,
                                             GtkTreeViewDropPosition  drop_pos);


#endif  /*  __GIMP_CONTAINER_TREE_VIEW_DND_H__  */
