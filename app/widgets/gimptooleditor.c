/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooleditor.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"
#include "gimptooleditor.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


typedef struct _GimpToolEditorPrivate GimpToolEditorPrivate;

struct _GimpToolEditorPrivate
{
  GtkTreeModel  *model;
  GimpContext   *context;
  GimpContainer *container;
  GtkWidget     *scrolled;

  GtkWidget     *raise_button;
  GtkWidget     *lower_button;
  GtkWidget     *reset_button;

  GQuark         visible_handler_id;
  GList         *default_tool_order;
};


static void   gimp_tool_editor_destroy     (GtkObject             *object);

static void   gimp_tool_editor_visible_notify
                                           (GimpToolInfo          *tool_info,
                                            GParamSpec            *pspec,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_eye_data_func
                                           (GtkTreeViewColumn     *tree_column,
                                            GtkCellRenderer       *cell,
                                            GtkTreeModel          *tree_model,
                                            GtkTreeIter           *iter,
                                            gpointer               data);
static void   gimp_tool_editor_eye_clicked (GtkCellRendererToggle *toggle,
                                            gchar                 *path_str,
                                            GdkModifierType        state,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_button_clicked
                                           (GtkButton             *button,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_button_extend_clicked
                                           (GtkButton             *button,
                                            GdkModifierType        mask,
                                            GimpToolEditor        *tool_editor);


G_DEFINE_TYPE (GimpToolEditor, gimp_tool_editor, GIMP_TYPE_CONTAINER_TREE_VIEW)

#define parent_class gimp_tool_editor_parent_class

#define GIMP_TOOL_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                           GIMP_TYPE_TOOL_EDITOR, \
                                           GimpToolEditorPrivate))


static void
gimp_tool_editor_class_init (GimpToolEditorClass *klass)
{
  GtkObjectClass           *object_class = GTK_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GimpToolEditorPrivate));

  object_class->destroy = gimp_tool_editor_destroy;
}

static void
gimp_tool_editor_init (GimpToolEditor *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  priv->model              = NULL;
  priv->context            = NULL;
  priv->container          = NULL;
  priv->scrolled           = NULL;

  priv->visible_handler_id = 0;
  priv->default_tool_order = NULL;

  priv->raise_button       = NULL;
  priv->lower_button       = NULL;
  priv->reset_button       = NULL;
}

static void
gimp_tool_editor_destroy (GtkObject *object)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (object);

  if (priv->visible_handler_id)
    {
      gimp_container_remove_handler (priv->container,
                                     priv->visible_handler_id);
      priv->visible_handler_id = 0;
    }

  priv->context            = NULL;
  priv->container          = NULL;

  priv->raise_button       = NULL;
  priv->lower_button       = NULL;
  priv->reset_button       = NULL;

  priv->scrolled           = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_tool_editor_new (GimpContainer *container,
                      GimpContext   *context,
                      GList         *default_tool_order,
                      gint           view_size,
                      gint           view_border_width)
{
  GimpToolEditor        *tool_editor;
  GimpContainerTreeView *tree_view;
  GimpContainerView     *container_view;
  GObject               *object;
  GimpToolEditorPrivate *priv;

  g_return_val_if_fail (container != NULL, NULL);
  g_return_val_if_fail (context   != NULL, NULL);

  object         = g_object_new (GIMP_TYPE_TOOL_EDITOR, NULL);
  tool_editor    = GIMP_TOOL_EDITOR (object);
  tree_view      = GIMP_CONTAINER_TREE_VIEW (object);
  container_view = GIMP_CONTAINER_VIEW (object);
  priv           = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  priv->container          = container;
  priv->context            = context;
  priv->model              = tree_view->model;
  priv->default_tool_order = default_tool_order;

  gimp_container_view_set_view_size (container_view,
                                     view_size, view_border_width);
  gimp_container_view_set_container (container_view, priv->container);
  gimp_container_view_set_context (container_view, context);
  gimp_container_view_set_reorderable (container_view, TRUE);
  gimp_editor_set_show_name (GIMP_EDITOR (tree_view), FALSE);

  /* Construct tree view */
  {
    GimpContainerTreeView *tree_view   = GIMP_CONTAINER_TREE_VIEW (tool_editor);
    GtkWidget             *tree_widget = GTK_WIDGET (tree_view);
    GtkStyle              *tree_style  = gtk_widget_get_style (tree_widget);
    GtkTreeViewColumn     *column;
    GtkCellRenderer       *eye_cell;
    GtkIconSize            icon_size;

    column    = gtk_tree_view_column_new ();
    gtk_tree_view_insert_column (tree_view->view, column, 0);
    eye_cell  = gimp_cell_renderer_toggle_new (GIMP_STOCK_VISIBLE);
    icon_size = gimp_get_icon_size (GTK_WIDGET (tool_editor),
                                    GIMP_STOCK_VISIBLE,
                                    GTK_ICON_SIZE_BUTTON,
                                    view_size -
                                    2 * tree_style->xthickness,
                                    view_size -
                                    2 * tree_style->ythickness);

    g_object_set (eye_cell, "stock-size", icon_size, NULL);
    gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
    gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                              gimp_tool_editor_eye_data_func,
                                              tree_view, NULL);

    gimp_container_tree_view_prepend_toggle_cell_renderer (tree_view,
                                                           eye_cell);

    g_signal_connect (eye_cell, "clicked",
                      G_CALLBACK (gimp_tool_editor_eye_clicked),
                      tool_editor);
    priv->visible_handler_id =
      gimp_container_add_handler (container, "notify::visible",
                                  G_CALLBACK (gimp_tool_editor_visible_notify),
                                  tool_editor);
  }

  /* buttons */
  priv->raise_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GTK_STOCK_GO_UP,
                            _("Raise this tool Raise this tool to the top"),
                            NULL,
                            G_CALLBACK (gimp_tool_editor_button_clicked),
                            G_CALLBACK (gimp_tool_editor_button_extend_clicked),
                            tool_editor);
  priv->lower_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GTK_STOCK_GO_DOWN,
                            _("Lower this tool Lower this tool to the bottom"),
                            NULL,
                            G_CALLBACK (gimp_tool_editor_button_clicked),
                            G_CALLBACK (gimp_tool_editor_button_extend_clicked),
                            tool_editor);
  priv->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GIMP_STOCK_RESET,
                            _("Reset tool order and visibility"), NULL,
                            G_CALLBACK (gimp_tool_editor_button_clicked), NULL,
                            tool_editor);

  return GTK_WIDGET (tool_editor);
}

static void
gimp_tool_editor_button_clicked (GtkButton    *button,
                                 GimpToolEditor *tool_editor)
{
  GimpToolInfo          *tool_info;
  GimpToolEditorPrivate *priv;
  gint                   index;

  priv      = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  tool_info = gimp_context_get_tool (priv->context);

  if (tool_info && button == GTK_BUTTON (priv->raise_button))
    {
      index = gimp_container_get_child_index (priv->container,
                                              GIMP_OBJECT (tool_info));

      if (index > 0)
        {
          gimp_container_reorder (priv->container,
                                  GIMP_OBJECT (tool_info), index - 1);
        }
    }
  else if (tool_info && button == GTK_BUTTON (priv->lower_button))
    {
      index = gimp_container_get_child_index (priv->container,
                                              GIMP_OBJECT (tool_info));

      if (index + 1 < gimp_container_get_n_children (priv->container))
        {
          gimp_container_reorder (priv->container,
                                  GIMP_OBJECT (tool_info), index + 1);
        }
    }
  else if (tool_info && button == GTK_BUTTON (priv->reset_button))
    {
      GList *list;
      gint   i    = 0;

      for (list = priv->default_tool_order;
           list;
           list = g_list_next (list))
        {
          GimpObject *object =
                 gimp_container_get_child_by_name (priv->container, list->data);

          if (object)
            {
              gboolean visible;
              gpointer data;

              gimp_container_reorder (priv->container, object, i);
              data = g_object_get_data (G_OBJECT (object),
                                        "gimp-tool-default-visible");

              visible = GPOINTER_TO_INT (data);
              g_object_set (object, "visible", visible, NULL);

              i++;
            }
        }
    }
}

static void
gimp_tool_editor_button_extend_clicked (GtkButton       *button,
                                        GdkModifierType  mask,
                                        GimpToolEditor    *tool_editor)
{
  GimpToolInfo          *tool_info;
  GimpToolEditorPrivate *priv;
  gint                   index;

  priv      = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  tool_info = gimp_context_get_tool (priv->context);

  if (! mask == GDK_SHIFT_MASK)
    {
      /* do nothing */
    }
  if (button == GTK_BUTTON (priv->raise_button))
    {
      index = gimp_container_get_child_index (priv->container,
                                              GIMP_OBJECT (tool_info));
      if (index > 0)
        gimp_container_reorder (priv->container,
                                GIMP_OBJECT (tool_info), 0);
    }
  else if (button == GTK_BUTTON (priv->lower_button))
    {
      index = gimp_container_get_n_children (priv->container) - 1;
      index = index >= 0 ? index : 0;

      gimp_container_reorder (priv->container,
                              GIMP_OBJECT (tool_info), index);
    }
}

static void
gimp_tool_editor_visible_notify (GimpToolInfo  *tool_info,
                                 GParamSpec    *pspec,
                                 GimpToolEditor  *tool_editor)
{
  GimpToolEditorPrivate *priv;
  GtkTreeIter           *iter;

  priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  iter = gimp_container_view_lookup (GIMP_CONTAINER_VIEW (tool_editor),
                                     GIMP_VIEWABLE (tool_info));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (priv->model, iter);

      gtk_tree_model_row_changed (priv->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
gimp_tool_editor_eye_data_func (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer   *cell,
                                GtkTreeModel      *tree_model,
                                GtkTreeIter       *iter,
                                gpointer           data)
{
  GimpViewRenderer *renderer;
  gboolean          visible;

  gtk_tree_model_get (tree_model, iter,
                      GIMP_CONTAINER_TREE_VIEW_COLUMN_RENDERER, &renderer,
                      -1);

  g_object_get (renderer->viewable, "visible", &visible, NULL);

  g_object_unref (renderer);

  g_object_set (cell, "active", visible, NULL);
}

static void
gimp_tool_editor_eye_clicked (GtkCellRendererToggle *toggle,
                              gchar                 *path_str,
                              GdkModifierType        state,
                              GimpToolEditor        *tool_editor)
{
  GtkTreeIter            iter;
  GtkTreePath           *path;
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (priv->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      gboolean          active;

      g_object_get (toggle,
                    "active", &active,
                    NULL);
      gtk_tree_model_get (priv->model, &iter,
                          GIMP_CONTAINER_TREE_VIEW_COLUMN_RENDERER, &renderer,
                          -1);

      g_object_set (renderer->viewable, "visible", ! active, NULL);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
