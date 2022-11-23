/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreeview.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_TREE_VIEW_H__
#define __LIGMA_CONTAINER_TREE_VIEW_H__


#include "ligmacontainerbox.h"


#define LIGMA_TYPE_CONTAINER_TREE_VIEW            (ligma_container_tree_view_get_type ())
#define LIGMA_CONTAINER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_TREE_VIEW, LigmaContainerTreeView))
#define LIGMA_CONTAINER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_TREE_VIEW, LigmaContainerTreeViewClass))
#define LIGMA_IS_CONTAINER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_TREE_VIEW))
#define LIGMA_IS_CONTAINER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_TREE_VIEW))
#define LIGMA_CONTAINER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_TREE_VIEW, LigmaContainerTreeViewClass))


typedef struct _LigmaContainerTreeViewClass   LigmaContainerTreeViewClass;
typedef struct _LigmaContainerTreeViewPrivate LigmaContainerTreeViewPrivate;

struct _LigmaContainerTreeView
{
  LigmaContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkTreeView                  *view;

  GtkTreeViewColumn            *main_column;
  GtkCellRenderer              *renderer_cell;

  Ligma                         *dnd_ligma; /* eek */

  LigmaContainerTreeViewPrivate *priv;
};

struct _LigmaContainerTreeViewClass
{
  LigmaContainerBoxClass  parent_class;

  /* signals */

  void     (* edit_name)      (LigmaContainerTreeView   *tree_view);

  /* virtual functions */

  gboolean (* drop_possible)  (LigmaContainerTreeView   *tree_view,
                               LigmaDndType              src_type,
                               GList                   *src_viewables,
                               LigmaViewable            *dest_viewable,
                               GtkTreePath             *drop_path,
                               GtkTreeViewDropPosition  drop_pos,
                               GtkTreeViewDropPosition *return_drop_pos,
                               GdkDragAction           *return_drag_action);
  void     (* drop_viewables) (LigmaContainerTreeView   *tree_view,
                               GList                   *src_viewables,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_color)     (LigmaContainerTreeView   *tree_view,
                               const LigmaRGB           *src_color,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_uri_list)  (LigmaContainerTreeView   *tree_view,
                               GList                   *uri_list,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_svg)       (LigmaContainerTreeView   *tree_view,
                               const gchar             *svg_data,
                               gsize                    svg_data_length,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_component) (LigmaContainerTreeView   *tree_view,
                               LigmaImage               *image,
                               LigmaChannelType          component,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_pixbuf)    (LigmaContainerTreeView   *tree_view,
                               GdkPixbuf               *pixbuf,
                               LigmaViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
};


GType       ligma_container_tree_view_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_container_tree_view_new      (LigmaContainer *container,
                                               LigmaContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);

GtkCellRenderer *
            ligma_container_tree_view_get_name_cell
                                              (LigmaContainerTreeView *tree_view);

void        ligma_container_tree_view_set_main_column_title
                                              (LigmaContainerTreeView *tree_view,
                                               const gchar           *title);

void        ligma_container_tree_view_add_toggle_cell
                                              (LigmaContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell);

void        ligma_container_tree_view_add_renderer_cell
                                              (LigmaContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell,
                                               gint                   column_number);

void        ligma_container_tree_view_set_dnd_drop_to_empty
                                              (LigmaContainerTreeView *tree_view,
                                               gboolean               dnd_drop_to_emtpy);
void        ligma_container_tree_view_connect_name_edited
                                              (LigmaContainerTreeView *tree_view,
                                               GCallback              callback,
                                               gpointer               data);
gboolean    ligma_container_tree_view_name_edited
                                              (GtkCellRendererText   *cell,
                                               const gchar           *path_str,
                                               const gchar           *new_name,
                                               LigmaContainerTreeView *tree_view);


#endif  /*  __LIGMA_CONTAINER_TREE_VIEW_H__  */
