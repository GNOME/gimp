/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreeview-dnd.h
 * Copyright (C) 2003-2009 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_TREE_VIEW_DND_H__
#define __LIGMA_CONTAINER_TREE_VIEW_DND_H__


void     ligma_container_tree_view_drag_failed (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               GtkDragResult          result,
                                               LigmaContainerTreeView *tree_view);
void     ligma_container_tree_view_drag_leave  (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               guint                  time,
                                               LigmaContainerTreeView *view);
gboolean ligma_container_tree_view_drag_motion (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               guint                  time,
                                               LigmaContainerTreeView *view);
gboolean ligma_container_tree_view_drag_drop   (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               guint                  time,
                                               LigmaContainerTreeView *view);
void     ligma_container_tree_view_drag_data_received
                                              (GtkWidget             *widget,
                                               GdkDragContext        *context,
                                               gint                   x,
                                               gint                   y,
                                               GtkSelectionData      *selection_data,
                                               guint                  info,
                                               guint                  time,
                                               LigmaContainerTreeView *view);

gboolean
ligma_container_tree_view_real_drop_possible (LigmaContainerTreeView   *tree_view,
                                             LigmaDndType              src_type,
                                             GList                   *src_viewables,
                                             LigmaViewable            *dest_viewable,
                                             GtkTreePath             *drop_path,
                                             GtkTreeViewDropPosition  drop_pos,
                                             GtkTreeViewDropPosition *return_drop_pos,
                                             GdkDragAction           *return_drag_action);
void
ligma_container_tree_view_real_drop_viewables (LigmaContainerTreeView   *tree_view,
                                              GList                   *src_viewables,
                                              LigmaViewable            *dest_viewable,
                                              GtkTreeViewDropPosition  drop_pos);


#endif  /*  __LIGMA_CONTAINER_TREE_VIEW_DND_H__  */
