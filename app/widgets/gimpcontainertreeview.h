/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreeview.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTAINER_TREE_VIEW_H__
#define __GIMP_CONTAINER_TREE_VIEW_H__


#include "gimpcontainerbox.h"


#define GIMP_TYPE_CONTAINER_TREE_VIEW            (gimp_container_tree_view_get_type ())
#define GIMP_CONTAINER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeView))
#define GIMP_CONTAINER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeViewClass))
#define GIMP_IS_CONTAINER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW))
#define GIMP_IS_CONTAINER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_TREE_VIEW))
#define GIMP_CONTAINER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_TREE_VIEW, GimpContainerTreeViewClass))


typedef struct _GimpContainerTreeViewClass GimpContainerTreeViewClass;

struct _GimpContainerTreeView
{
  GimpContainerBox   parent_instance;

  GtkTreeModel      *model;
  gint               n_model_columns;
  GType              model_columns[16];

  gint               model_column_renderer;
  gint               model_column_name;
  gint               model_column_name_attributes;

  GtkTreeView       *view;
  GtkTreeSelection  *selection;

  GtkTreeViewColumn *main_column;
  GtkCellRenderer   *renderer_cell;
  GtkCellRenderer   *name_cell;

  GList             *toggle_cells;
  GList             *renderer_cells;
  GList             *editable_cells;

  GQuark             invalidate_preview_handler_id;

  gboolean           dnd_drop_to_empty;
  Gimp              *dnd_gimp; /* eek */
  GimpViewRenderer  *dnd_renderer;

  guint              scroll_timeout_id;
  guint              scroll_timeout_interval;
  GdkScrollDirection scroll_dir;
};

struct _GimpContainerTreeViewClass
{
  GimpContainerBoxClass  parent_class;

  gboolean (* drop_possible)  (GimpContainerTreeView   *tree_view,
                               GimpDndType              src_type,
                               GimpViewable            *src_viewable,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos,
                               GtkTreeViewDropPosition *return_drop_pos,
                               GdkDragAction           *return_drag_action);
  void     (* drop_viewable)  (GimpContainerTreeView   *tree_view,
                               GimpViewable            *src_viewable,
                               GimpViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_color)     (GimpContainerTreeView   *tree_view,
                               const GimpRGB           *src_color,
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


#endif  /*  __GIMP_CONTAINER_TREE_VIEW_H__  */
