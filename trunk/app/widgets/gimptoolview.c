/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"
#include "gimptoolview.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_tool_view_destroy        (GtkObject             *object);

static void   gimp_tool_view_select_item    (GimpContainerEditor   *editor,
                                             GimpViewable          *viewable);
static void   gimp_tool_view_activate_item  (GimpContainerEditor   *editor,
                                             GimpViewable          *viewable);

static void   gimp_tool_view_visible_notify (GimpToolInfo          *tool_info,
                                             GParamSpec            *pspec,
                                             GimpContainerTreeView *tree_view);
static void   gimp_tool_view_eye_data_func  (GtkTreeViewColumn     *tree_column,
                                             GtkCellRenderer       *cell,
                                             GtkTreeModel          *tree_model,
                                             GtkTreeIter           *iter,
                                             gpointer               data);
static void   gimp_tool_view_eye_clicked    (GtkCellRendererToggle *toggle,
                                             gchar                 *path_str,
                                             GdkModifierType        state,
                                             GimpContainerTreeView *tree_view);


G_DEFINE_TYPE (GimpToolView, gimp_tool_view, GIMP_TYPE_CONTAINER_EDITOR)

#define parent_class gimp_tool_view_parent_class


static void
gimp_tool_view_class_init (GimpToolViewClass *klass)
{
  GtkObjectClass           *object_class = GTK_OBJECT_CLASS (klass);
  GimpContainerEditorClass *editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  object_class->destroy       = gimp_tool_view_destroy;

  editor_class->select_item   = gimp_tool_view_select_item;
  editor_class->activate_item = gimp_tool_view_activate_item;
}

static void
gimp_tool_view_init (GimpToolView *view)
{
  view->visible_handler_id = 0;
  view->raise_button       = NULL;
  view->lower_button       = NULL;
  view->reset_button       = NULL;
}

static void
gimp_tool_view_destroy (GtkObject *object)
{
  GimpToolView *tool_view = GIMP_TOOL_VIEW (object);

  if (tool_view->visible_handler_id)
    {
      GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (tool_view);
      GimpContainerView   *view   = GIMP_CONTAINER_VIEW (editor->view);

      gimp_container_remove_handler (gimp_container_view_get_container (view),
                                     tool_view->visible_handler_id);
      tool_view->visible_handler_id = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_tool_view_new (GimpViewType     view_type,
                    GimpContainer   *container,
                    GimpContext     *context,
                    gint             view_size,
                    gint             view_border_width,
                    GimpMenuFactory *menu_factory)
{
  GimpToolView        *tool_view;
  GimpContainerEditor *editor;

  tool_view = g_object_new (GIMP_TYPE_TOOL_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (tool_view),
                                         view_type,
                                         container, context,
                                         view_size, view_border_width,
                                         menu_factory, "<Tools>",
                                         "/tools-popup"))
    {
      g_object_unref (tool_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (tool_view);

  tool_view->raise_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "tools",
                                   "tools-raise",
                                   "tools-raise-to-top", GDK_SHIFT_MASK,
                                   NULL);

  tool_view->lower_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "tools",
                                   "tools-lower",
                                   "tools-lower-to-bottom", GDK_SHIFT_MASK,
                                    NULL);

  tool_view->reset_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor->view), "tools",
                                   "tools-reset", NULL);

  if (view_type == GIMP_VIEW_TYPE_LIST)
    {
      GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);
      GtkWidget             *tree_widget = GTK_WIDGET (tree_view);
      GtkTreeViewColumn     *column;
      GtkCellRenderer       *eye_cell;
      GtkIconSize            icon_size;

      column = gtk_tree_view_column_new ();
      gtk_tree_view_insert_column (tree_view->view, column, 0);

      eye_cell = gimp_cell_renderer_toggle_new (GIMP_STOCK_VISIBLE);

      icon_size = gimp_get_icon_size (GTK_WIDGET (tree_view),
                                      GIMP_STOCK_VISIBLE,
                                      GTK_ICON_SIZE_BUTTON,
                                      view_size -
                                      2 * tree_widget->style->xthickness,
                                      view_size -
                                      2 * tree_widget->style->ythickness);
      g_object_set (eye_cell, "stock-size", icon_size, NULL);

      gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
      gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                                gimp_tool_view_eye_data_func,
                                                tree_view,
                                                NULL);

      tree_view->toggle_cells = g_list_prepend (tree_view->toggle_cells,
                                                eye_cell);

      g_signal_connect (eye_cell, "clicked",
                        G_CALLBACK (gimp_tool_view_eye_clicked),
                        tree_view);

      tool_view->visible_handler_id =
        gimp_container_add_handler (container, "notify::visible",
                                    G_CALLBACK (gimp_tool_view_visible_notify),
                                    tree_view);
    }

  gimp_ui_manager_update (GIMP_EDITOR (editor->view)->ui_manager, editor);

  return GTK_WIDGET (tool_view);
}

static void
gimp_tool_view_select_item (GimpContainerEditor *editor,
                            GimpViewable        *viewable)
{
  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);
}

static void
gimp_tool_view_activate_item (GimpContainerEditor *editor,
                              GimpViewable        *viewable)
{
  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);
}


/*  "visible" callbaks  */

static void
gimp_tool_view_visible_notify (GimpToolInfo          *tool_info,
                               GParamSpec            *pspec,
                               GimpContainerTreeView *tree_view)
{
  GtkTreeIter *iter;

  iter = gimp_container_view_lookup (GIMP_CONTAINER_VIEW (tree_view),
                                     (GimpViewable *) tool_info);

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
gimp_tool_view_eye_data_func (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *tree_model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (data);
  GimpViewRenderer      *renderer;
  gboolean               visible;

  gtk_tree_model_get (tree_model, iter,
                      tree_view->model_column_renderer, &renderer,
                      -1);

  g_object_get (renderer->viewable, "visible", &visible, NULL);

  g_object_unref (renderer);

  g_object_set (cell, "active", visible, NULL);
}

static void
gimp_tool_view_eye_clicked (GtkCellRendererToggle *toggle,
                            gchar                 *path_str,
                            GdkModifierType        state,
                            GimpContainerTreeView *tree_view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      gboolean          active;

      g_object_get (toggle,
                    "active", &active,
                    NULL);

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      g_object_set (renderer->viewable, "visible", ! active, NULL);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
