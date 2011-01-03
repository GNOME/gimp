/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooleditor.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
 *                         Stephen Griffiths <scgmk5@gmail.com>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"
#include "gimptooleditor.h"
#include "gimphelp-ids.h"
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

  /* State of tools at creation of the editor, stored to support
   * reverting changes
   */
  gchar        **initial_tool_order;
  gboolean      *initial_tool_visibility;
  gint           n_tools;

  GQuark         visible_handler_id;
  GList         *default_tool_order;
};


static void   gimp_tool_editor_dispose     (GObject               *object);
static void   gimp_tool_editor_finalize    (GObject               *object);

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

static void   gimp_tool_editor_raise_clicked
                                           (GtkButton             *button,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_raise_extend_clicked
                                           (GtkButton             *button,
                                            GdkModifierType        mask,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_lower_clicked
                                           (GtkButton             *button,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_lower_extend_clicked
                                           (GtkButton             *button,
                                            GdkModifierType        mask,
                                            GimpToolEditor        *tool_editor);
static void   gimp_tool_editor_reset_clicked
                                           (GtkButton             *button,
                                            GimpToolEditor        *tool_editor);


G_DEFINE_TYPE (GimpToolEditor, gimp_tool_editor, GIMP_TYPE_CONTAINER_TREE_VIEW)

#define parent_class gimp_tool_editor_parent_class

#define GIMP_TOOL_EDITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                           GIMP_TYPE_TOOL_EDITOR, \
                                           GimpToolEditorPrivate))


static void
gimp_tool_editor_class_init (GimpToolEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_tool_editor_dispose;
  object_class->finalize = gimp_tool_editor_finalize;

  g_type_class_add_private (klass, sizeof (GimpToolEditorPrivate));
}

static void
gimp_tool_editor_init (GimpToolEditor *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  priv->model                   = NULL;
  priv->context                 = NULL;
  priv->container               = NULL;
  priv->scrolled                = NULL;

  priv->visible_handler_id      = 0;
  priv->default_tool_order      = NULL;

  priv->initial_tool_order      = NULL;
  priv->initial_tool_visibility = NULL;
  priv->n_tools                 = 0;

  priv->raise_button            = NULL;
  priv->lower_button            = NULL;
  priv->reset_button            = NULL;
}

static void
gimp_tool_editor_dispose (GObject *object)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (object);

  if (priv->visible_handler_id)
    {
      gimp_container_remove_handler (priv->container,
                                     priv->visible_handler_id);
      priv->visible_handler_id = 0;
    }

  priv->context      = NULL;
  priv->container    = NULL;

  priv->raise_button = NULL;
  priv->lower_button = NULL;
  priv->reset_button = NULL;

  priv->scrolled     = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_editor_finalize (GObject *object)
{
  GimpToolEditor *tool_editor;
  GimpToolEditorPrivate *priv;

  tool_editor = GIMP_TOOL_EDITOR (object);
  priv        = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  if (priv->initial_tool_order)
    {
      int i;

      for (i = 0; i < priv->n_tools; i++)
        {
          g_free (priv->initial_tool_order[i]);
        }

      g_free (priv->initial_tool_order);
      priv->initial_tool_order      = NULL;
    }

  if (priv->initial_tool_visibility)
    {
      g_slice_free1 (sizeof (gboolean) * priv->n_tools,
                     priv->initial_tool_visibility);

      priv->initial_tool_visibility = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gimp_tool_editor_new (GimpContainer *container,
                      GimpContext   *context,
                      GList         *default_tool_order,
                      gint           view_size,
                      gint           view_border_width)
{
  int                    i;
  GimpToolEditor        *tool_editor;
  GimpContainerTreeView *tree_view;
  GimpContainerView     *container_view;
  GObject               *object;
  GimpObject            *gimp_object;
  GimpToolEditorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  object         = g_object_new (GIMP_TYPE_TOOL_EDITOR, NULL);
  tool_editor    = GIMP_TOOL_EDITOR (object);
  tree_view      = GIMP_CONTAINER_TREE_VIEW (object);
  container_view = GIMP_CONTAINER_VIEW (object);
  priv           = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  priv->container               = container;
  priv->context                 = context;
  priv->model                   = tree_view->model;
  priv->default_tool_order      = default_tool_order;
  priv->initial_tool_order      = gimp_container_get_name_array (container,
                                                                 &priv->n_tools);
  priv->initial_tool_visibility = g_slice_alloc (sizeof (gboolean) *
                                                 priv->n_tools);
  for (i = 0; i < priv->n_tools; i++)
    {
      gimp_object = gimp_container_get_child_by_index (container, i);

      g_object_get (gimp_object,
                    "visible", &(priv->initial_tool_visibility[i]), NULL);
    }

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
    GtkStyleContext       *tree_style;
    GtkTreeViewColumn     *column;
    GtkCellRenderer       *eye_cell;
    GtkBorder              border;
    GtkIconSize            icon_size;

    tree_style = gtk_widget_get_style_context (tree_widget);
    gtk_style_context_get_border (tree_style, 0, &border);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_insert_column (tree_view->view, column, 0);

    eye_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_VISIBLE);

    icon_size = gimp_get_icon_size (GTK_WIDGET (tool_editor),
                                    GIMP_ICON_VISIBLE,
                                    GTK_ICON_SIZE_BUTTON,
                                    view_size - (border.left + border.right),
                                    view_size - (border.top + border.bottom));

    g_object_set (eye_cell, "stock-size", icon_size, NULL);
    gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
    gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                              gimp_tool_editor_eye_data_func,
                                              tree_view, NULL);

    gimp_container_tree_view_add_toggle_cell (tree_view, eye_cell);

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
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GIMP_ICON_GO_UP,
                            _("Raise this tool"),
                            _("Raise this tool to the top"),
                            G_CALLBACK (gimp_tool_editor_raise_clicked),
                            G_CALLBACK (gimp_tool_editor_raise_extend_clicked),
                            tool_editor);

  priv->lower_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GIMP_ICON_GO_DOWN,
                            _("Lower this tool"),
                            _("Lower this tool to the bottom"),
                            G_CALLBACK (gimp_tool_editor_lower_clicked),
                            G_CALLBACK (gimp_tool_editor_lower_extend_clicked),
                            tool_editor);

  priv->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (tree_view), GIMP_ICON_RESET,
                            _("Reset tool order and visibility"), NULL,
                            G_CALLBACK (gimp_tool_editor_reset_clicked), NULL,
                            tool_editor);

  return GTK_WIDGET (tool_editor);
}

/**
 * gimp_tool_editor_revert_changes:
 * @tool_editor:
 *
 * Reverts the tool order and visibility to the state at creation.
 **/
void
gimp_tool_editor_revert_changes (GimpToolEditor *tool_editor)
{
  int i;
  GimpToolEditorPrivate *priv;

  priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);

  for (i = 0; i < priv->n_tools; i++)
    {
      GimpObject *object;

      object = gimp_container_get_child_by_name (priv->container,
                                                 priv->initial_tool_order[i]);

      gimp_container_reorder (priv->container, object, i);
      g_object_set (object, "visible", priv->initial_tool_visibility[i], NULL);
    }
}

static void
gimp_tool_editor_raise_clicked (GtkButton    *button,
                                GimpToolEditor *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GimpToolInfo          *tool_info;

  tool_info = gimp_context_get_tool (priv->context);

  if (tool_info)
    {
      gint index = gimp_container_get_child_index (priv->container,
                                                   GIMP_OBJECT (tool_info));

      if (index > 0)
        {
          gimp_container_reorder (priv->container,
                                  GIMP_OBJECT (tool_info), index - 1);
        }
    }
}

static void
gimp_tool_editor_raise_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       GimpToolEditor    *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GimpToolInfo          *tool_info;

  tool_info = gimp_context_get_tool (priv->context);

  if (tool_info && (mask & GDK_SHIFT_MASK))
    {
      gint index = gimp_container_get_child_index (priv->container,
                                                   GIMP_OBJECT (tool_info));

      if (index > 0)
        {
          gimp_container_reorder (priv->container,
                                  GIMP_OBJECT (tool_info), 0);
        }
    }
}

static void
gimp_tool_editor_lower_clicked (GtkButton    *button,
                                GimpToolEditor *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GimpToolInfo          *tool_info;

  tool_info = gimp_context_get_tool (priv->context);

  if (tool_info)
    {
      gint index = gimp_container_get_child_index (priv->container,
                                                   GIMP_OBJECT (tool_info));

      if (index + 1 < gimp_container_get_n_children (priv->container))
        {
          gimp_container_reorder (priv->container,
                                  GIMP_OBJECT (tool_info), index + 1);
        }
    }
}

static void
gimp_tool_editor_lower_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       GimpToolEditor    *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GimpToolInfo          *tool_info;

  tool_info = gimp_context_get_tool (priv->context);

  if (tool_info && (mask & GDK_SHIFT_MASK))
    {
      gint index = gimp_container_get_n_children (priv->container) - 1;

      index = MAX (index, 0);

      gimp_container_reorder (priv->container,
                              GIMP_OBJECT (tool_info), index);
    }
}

static void
gimp_tool_editor_reset_clicked (GtkButton    *button,
                                GimpToolEditor *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GList                 *list;
  gint                   i;

  for (list = priv->default_tool_order, i = 0;
       list;
       list = g_list_next (list), i++)
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
        }
    }
}

static void
gimp_tool_editor_visible_notify (GimpToolInfo  *tool_info,
                                 GParamSpec    *pspec,
                                 GimpToolEditor  *tool_editor)
{
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GtkTreeIter           *iter;

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
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
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
  GimpToolEditorPrivate *priv = GIMP_TOOL_EDITOR_GET_PRIVATE (tool_editor);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (priv->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      gboolean          active;

      g_object_get (toggle,
                    "active", &active,
                    NULL);
      gtk_tree_model_get (priv->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      g_object_set (renderer->viewable, "visible", ! active, NULL);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
