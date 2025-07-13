/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpcontainerbox.h"


#define GIMP_TYPE_CONTAINER_TREE_VIEW            (gimp_container_tree_view_get_type ())
#define GIMP_CONTAINER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeView))
#define GIMP_CONTAINER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeViewClass))
#define GIMP_IS_CONTAINER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW))
#define GIMP_IS_CONTAINER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_TREE_VIEW))
#define GIMP_CONTAINER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeViewClass))


typedef struct _GimpContainerTreeViewClass   GimpContainerTreeViewClass;
typedef struct _GimpContainerTreeViewPrivate GimpContainerTreeViewPrivate;

struct _GimpContainerTreeView
{
  GimpContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkTreeView                  *view;

  GtkTreeViewColumn            *main_column;
  GtkCellRenderer              *renderer_cell;

  Gimp                         *dnd_gimp; /* eek */

  GimpContainerTreeViewPrivate *priv;
};

struct _GimpContainerTreeViewClass
{
  GimpContainerBoxClass  parent_class;

  /* signals */

  void     (* edit_name)      (GimpContainerTreeView   *tree_view);

  /* virtual functions */

  gboolean (* drop_possible)  (GimpContainerTreeView   *tree_view,
                               GimpDndType              src_type,
                               GList                   *src_viewables,
                               GimpViewable            *dest_viewable,
                               GtkTreePath             *drop_path,
                               GtkTreeViewDropPosition  drop_pos,
                               GtkTreeViewDropPosition *return_drop_pos,
                               GdkDragAction           *return_drag_action);
  void     (* drop_viewables) (GimpContainerTreeView   *tree_view,
                               GList                   *src_viewables,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_color)     (GimpContainerTreeView   *tree_view,
                               GeglColor               *src_color,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_uri_list)  (GimpContainerTreeView   *tree_view,
                               GList                   *uri_list,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_svg)       (GimpContainerTreeView   *tree_view,
                               const gchar             *svg_data,
                               gsize                    svg_data_length,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_component) (GimpContainerTreeView   *tree_view,
                               GimpImage               *image,
                               GimpChannelType          component,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_pixbuf)    (GimpContainerTreeView   *tree_view,
                               GdkPixbuf               *pixbuf,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
};


GType       gimp_container_tree_view_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_tree_view_new      (GimpContainer *container,
                                               GimpContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);

GtkCellRenderer *
            gimp_container_tree_view_get_name_cell
                                              (GimpContainerTreeView *tree_view);

void        gimp_container_tree_view_set_main_column_title
                                              (GimpContainerTreeView *tree_view,
                                               const gchar           *title);

void        gimp_container_tree_view_add_toggle_cell
                                              (GimpContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell);

void        gimp_container_tree_view_add_renderer_cell
                                              (GimpContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell,
                                               gint                   column_number);

void        gimp_container_tree_view_set_dnd_drop_to_empty
                                              (GimpContainerTreeView *tree_view,
                                               gboolean               dnd_drop_to_emtpy);
void        gimp_container_tree_view_connect_name_edited
                                              (GimpContainerTreeView *tree_view,
                                               GCallback              callback,
                                               gpointer               data);
gboolean    gimp_container_tree_view_name_edited
                                              (GtkCellRendererText   *cell,
                                               const gchar           *path_str,
                                               const gchar           *new_name,
                                               GimpContainerTreeView *tree_view);
