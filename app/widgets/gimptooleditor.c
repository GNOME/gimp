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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptoolgroup.h"
#include "core/gimptreehandler.h"

#include "tools/gimp-tools.h"

#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpviewrenderer.h"
#include "gimptooleditor.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


struct _GimpToolEditorPrivate
{
  GimpContainer   *container;
  GimpContext     *context;

  GtkWidget       *scrolled;

  GtkWidget       *new_group_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  GimpTreeHandler *tool_item_notify_handler;

  /* State of tools at creation of the editor, stored to support
   * reverting changes
   */
  gchar           *initial_tool_state;
};


/*  local function prototypes  */

static void            gimp_tool_editor_view_iface_init           (GimpContainerViewInterface *iface);

static void            gimp_tool_editor_constructed               (GObject                    *object);

static gboolean        gimp_tool_editor_select_item               (GimpContainerView          *view,
                                                                   GimpViewable               *viewable,
                                                                   gpointer                    insert_data);
static void            gimp_tool_editor_set_container             (GimpContainerView          *container_view,
                                                                   GimpContainer              *container);
static void            gimp_tool_editor_set_context               (GimpContainerView          *container_view,
                                                                   GimpContext                *context);

static gboolean        gimp_tool_editor_drop_possible             (GimpContainerTreeView      *tree_view,
                                                                   GimpDndType                 src_type,
                                                                   GimpViewable               *src_viewable,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void            gimp_tool_editor_drop_viewable             (GimpContainerTreeView      *tree_view,
                                                                   GimpViewable               *src_viewable,
                                                                   GimpViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);

static void            gimp_tool_editor_tool_item_notify         (GimpToolItem                *tool_item,
                                                                  GParamSpec                  *pspec,
                                                                  GimpToolEditor              *tool_editor);

static void            gimp_tool_editor_eye_data_func            (GtkTreeViewColumn           *tree_column,
                                                                  GtkCellRenderer             *cell,
                                                                  GtkTreeModel                *tree_model,
                                                                  GtkTreeIter                 *iter,
                                                                  gpointer                     data);
static void            gimp_tool_editor_eye_clicked              (GtkCellRendererToggle       *toggle,
                                                                  gchar                       *path_str,
                                                                  GdkModifierType              state,
                                                                  GimpToolEditor              *tool_editor);

static void            gimp_tool_editor_new_group_clicked        (GtkButton                   *button,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_raise_clicked            (GtkButton                   *button,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_raise_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_lower_clicked            (GtkButton                   *button,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_lower_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_delete_clicked           (GtkButton                   *button,
                                                                  GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_reset_clicked            (GtkButton                   *button,
                                                                  GimpToolEditor              *tool_editor);

static GimpToolItem  * gimp_tool_editor_get_selected_tool_item   (GimpToolEditor              *tool_editor);
static GimpContainer * gimp_tool_editor_get_tool_item_container  (GimpToolEditor              *tool_editor,
                                                                  GimpToolItem                *tool_item);

static void            gimp_tool_editor_update_container         (GimpToolEditor              *tool_editor);
static void            gimp_tool_editor_update_sensitivity       (GimpToolEditor              *tool_editor);


G_DEFINE_TYPE_WITH_CODE (GimpToolEditor, gimp_tool_editor,
                         GIMP_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (GimpToolEditor)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_tool_editor_view_iface_init))

#define parent_class gimp_tool_editor_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


/*  private functions  */

static void
gimp_tool_editor_class_init (GimpToolEditorClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GimpContainerTreeViewClass *tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);

  object_class->constructed      = gimp_tool_editor_constructed;

  tree_view_class->drop_possible = gimp_tool_editor_drop_possible;
  tree_view_class->drop_viewable = gimp_tool_editor_drop_viewable;
}

static void
gimp_tool_editor_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (GIMP_TYPE_CONTAINER_VIEW);

  iface->select_item   = gimp_tool_editor_select_item;
  iface->set_container = gimp_tool_editor_set_container;
  iface->set_context   = gimp_tool_editor_set_context;
}

static void
gimp_tool_editor_init (GimpToolEditor *tool_editor)
{
  tool_editor->priv = gimp_tool_editor_get_instance_private (tool_editor);
}

static void
gimp_tool_editor_constructed (GObject *object)
{
  GimpToolEditor        *tool_editor    = GIMP_TOOL_EDITOR (object);
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (object);
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (object);
  gint                   view_size;
  gint                   border_width;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view_size = gimp_container_view_get_view_size (container_view,
                                                 &border_width);

  gimp_editor_set_show_name (GIMP_EDITOR (tool_editor), FALSE);

  gtk_tree_view_set_level_indentation (tree_view->view,
                                       0.8 * (view_size + 2 * border_width));

  gimp_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view),
                              GIMP_TYPE_TOOL_ITEM,
                              NULL, NULL);

  /* construct tree view */
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer   *eye_cell;
    GtkStyle          *tree_style;
    GtkIconSize        icon_size;

    tree_style = gtk_widget_get_style (GTK_WIDGET (tool_editor));

    icon_size = gimp_get_icon_size (GTK_WIDGET (tool_editor),
                                    GIMP_ICON_VISIBLE,
                                    GTK_ICON_SIZE_BUTTON,
                                    view_size -
                                    2 * tree_style->xthickness,
                                    view_size -
                                    2 * tree_style->ythickness);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_insert_column (tree_view->view, column, 0);

    eye_cell = gimp_cell_renderer_toggle_new (GIMP_ICON_VISIBLE);
    g_object_set (eye_cell, "stock-size", icon_size, NULL);
    gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
    gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                              gimp_tool_editor_eye_data_func,
                                              tree_view, NULL);

    gimp_container_tree_view_add_toggle_cell (tree_view, eye_cell);

    g_signal_connect (eye_cell, "clicked",
                      G_CALLBACK (gimp_tool_editor_eye_clicked),
                      tool_editor);
  }

  /* buttons */
  tool_editor->priv->new_group_button =
    gimp_editor_add_button (GIMP_EDITOR (tool_editor), GIMP_ICON_FOLDER_NEW,
                            _("Create a new tool group"), NULL,
                            G_CALLBACK (gimp_tool_editor_new_group_clicked),
                            NULL,
                            tool_editor);

  tool_editor->priv->raise_button =
    gimp_editor_add_button (GIMP_EDITOR (tool_editor), GIMP_ICON_GO_UP,
                            _("Raise this tool"),
                            _("Raise this tool to the top"),
                            G_CALLBACK (gimp_tool_editor_raise_clicked),
                            G_CALLBACK (gimp_tool_editor_raise_extend_clicked),
                            tool_editor);

  tool_editor->priv->lower_button =
    gimp_editor_add_button (GIMP_EDITOR (tool_editor), GIMP_ICON_GO_DOWN,
                            _("Lower this tool"),
                            _("Lower this tool to the bottom"),
                            G_CALLBACK (gimp_tool_editor_lower_clicked),
                            G_CALLBACK (gimp_tool_editor_lower_extend_clicked),
                            tool_editor);

  tool_editor->priv->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (tool_editor), GIMP_ICON_EDIT_DELETE,
                            _("Delete this tool"), NULL,
                            G_CALLBACK (gimp_tool_editor_delete_clicked),
                            NULL,
                            tool_editor);

  tool_editor->priv->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (tool_editor), GIMP_ICON_RESET,
                            _("Reset tool order and visibility"), NULL,
                            G_CALLBACK (gimp_tool_editor_reset_clicked),
                            NULL,
                            tool_editor);

  gimp_tool_editor_update_sensitivity (tool_editor);
}

static gboolean
gimp_tool_editor_select_item (GimpContainerView *container_view,
                              GimpViewable      *viewable,
                              gpointer           insert_data)
{
  GimpToolEditor *tool_editor = GIMP_TOOL_EDITOR (container_view);
  gboolean        result;

  result = parent_view_iface->select_item (container_view,
                                           viewable, insert_data);

  gimp_tool_editor_update_sensitivity (tool_editor);

  return result;
}

static void
gimp_tool_editor_set_container (GimpContainerView *container_view,
                                GimpContainer     *container)
{
  GimpToolEditor *tool_editor = GIMP_TOOL_EDITOR (container_view);

  parent_view_iface->set_container (container_view, container);

  gimp_tool_editor_update_container (tool_editor);
}

static void
gimp_tool_editor_set_context (GimpContainerView *container_view,
                              GimpContext       *context)
{
  GimpToolEditor *tool_editor = GIMP_TOOL_EDITOR (container_view);

  parent_view_iface->set_context (container_view, context);

  gimp_tool_editor_update_container (tool_editor);
}

static gboolean
gimp_tool_editor_drop_possible (GimpContainerTreeView   *tree_view,
                                GimpDndType              src_type,
                                GimpViewable            *src_viewable,
                                GimpViewable            *dest_viewable,
                                GtkTreePath             *drop_path,
                                GtkTreeViewDropPosition  drop_pos,
                                GtkTreeViewDropPosition *return_drop_pos,
                                GdkDragAction           *return_drag_action)
{
  if (GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (
        tree_view,
        src_type, src_viewable, dest_viewable, drop_path, drop_pos,
        return_drop_pos, return_drag_action))
    {
      if (gimp_viewable_get_parent (dest_viewable)       ||
          (gimp_viewable_get_children (dest_viewable)    &&
           (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
            drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
        {
          return ! gimp_viewable_get_children (src_viewable);
        }

      return TRUE;
    }

  return FALSE;
}

static void
gimp_tool_editor_drop_viewable (GimpContainerTreeView   *tree_view,
                                GimpViewable            *src_viewable,
                                GimpViewable            *dest_viewable,
                                GtkTreeViewDropPosition  drop_pos)
{
  GimpContainerView *container_view = GIMP_CONTAINER_VIEW (tree_view);

  GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewable (tree_view,
                                                                src_viewable,
                                                                dest_viewable,
                                                                drop_pos);

  gimp_container_view_select_item (container_view, src_viewable);
}

static void
gimp_tool_editor_new_group_clicked (GtkButton      *button,
                                    GimpToolEditor *tool_editor)
{
  GimpContainerView *container_view = GIMP_CONTAINER_VIEW (tool_editor);
  GimpContainer     *container;
  GimpToolItem      *tool_item;
  GimpToolGroup     *group;
  gint               index = 0;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      if (gimp_viewable_get_parent (GIMP_VIEWABLE (tool_item)) != NULL)
        return;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (tool_item));
    }
  else
    {
      container = tool_editor->priv->container;
    }

  if (container)
    {
      group = gimp_tool_group_new ();

      gimp_container_insert (container, GIMP_OBJECT (group), index);

      g_object_unref (group);

      gimp_container_view_select_item (container_view, GIMP_VIEWABLE (group));
    }
}

static void
gimp_tool_editor_raise_clicked (GtkButton      *button,
                                GimpToolEditor *tool_editor)
{
  GimpToolItem *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      GimpContainer *container;
      gint           index;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (tool_item));

      if (index > 0)
        {
          gimp_container_reorder (container,
                                  GIMP_OBJECT (tool_item), index - 1);
        }
    }
}

static void
gimp_tool_editor_raise_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       GimpToolEditor  *tool_editor)
{
  GimpToolItem *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      GimpContainer *container;
      gint           index;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (tool_item));

      if (index > 0)
        {
          gimp_container_reorder (container,
                                  GIMP_OBJECT (tool_item), 0);
        }
    }
}

static void
gimp_tool_editor_lower_clicked (GtkButton      *button,
                                GimpToolEditor *tool_editor)
{
  GimpToolItem *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      GimpContainer *container;
      gint           index;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (tool_item));

      if (index + 1 < gimp_container_get_n_children (container))
        {
          gimp_container_reorder (container,
                                  GIMP_OBJECT (tool_item), index + 1);
        }
    }
}

static void
gimp_tool_editor_lower_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       GimpToolEditor  *tool_editor)
{
  GimpToolItem *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      GimpContainer *container;
      gint           index;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_n_children (container) - 1;
      index = MAX (index, 0);

      gimp_container_reorder (container,
                              GIMP_OBJECT (tool_item), index);
    }
}

static void
gimp_tool_editor_delete_clicked (GtkButton      *button,
                                 GimpToolEditor *tool_editor)
{
  GimpContainerView *container_view = GIMP_CONTAINER_VIEW (tool_editor);
  GimpToolItem     *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      GimpContainer *src_container;
      GimpContainer *dest_container;
      gint           index;
      gint           dest_index;

      src_container  = gimp_viewable_get_children (GIMP_VIEWABLE (tool_item));
      dest_container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                                 tool_item);

      if (! src_container)
        return;

      index      = gimp_container_get_child_index (dest_container,
                                                   GIMP_OBJECT (tool_item));
      dest_index = index;

      g_object_ref (tool_item);

      gimp_container_freeze (src_container);
      gimp_container_freeze (dest_container);

      gimp_container_remove (dest_container, GIMP_OBJECT (tool_item));

      while (! gimp_container_is_empty (src_container))
        {
          GimpObject *object = gimp_container_get_first_child (src_container);

          g_object_ref (object);

          gimp_container_remove (src_container,  object);
          gimp_container_insert (dest_container, object, dest_index++);

          g_object_unref (object);
        }

      gimp_container_thaw (dest_container);
      gimp_container_thaw (src_container);

      gimp_container_view_select_item (
        container_view,
        GIMP_VIEWABLE (gimp_container_get_child_by_index (dest_container,
                                                          index)));

      g_object_unref (tool_item);
    }
}

static void
gimp_tool_editor_reset_clicked (GtkButton      *button,
                                GimpToolEditor *tool_editor)
{
  gimp_tools_reset (tool_editor->priv->context->gimp,
                    tool_editor->priv->container,
                    FALSE);
}

static void
gimp_tool_editor_tool_item_notify (GimpToolItem   *tool_item,
                                   GParamSpec     *pspec,
                                   GimpToolEditor *tool_editor)
{
  GimpContainerTreeView *tree_view      = GIMP_CONTAINER_TREE_VIEW (tool_editor);
  GimpContainerView     *container_view = GIMP_CONTAINER_VIEW (tool_editor);
  GtkTreeIter           *iter;

  iter = gimp_container_view_lookup (container_view,
                                     GIMP_VIEWABLE (tool_item));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

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
  GimpToolItem     *tool_item;

  gtk_tree_model_get (tree_model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  tool_item = GIMP_TOOL_ITEM (renderer->viewable);

  g_object_set (cell,
                "active",       gimp_tool_item_get_visible (tool_item),
                "inconsistent", gimp_tool_item_get_visible (tool_item) &&
                                ! gimp_tool_item_get_shown (tool_item),
                NULL);

  g_object_unref (renderer);
}

static void
gimp_tool_editor_eye_clicked (GtkCellRendererToggle *toggle,
                              gchar                 *path_str,
                              GdkModifierType        state,
                              GimpToolEditor        *tool_editor)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (tool_editor);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpViewRenderer *renderer;
      GimpToolItem     *tool_item;
      gboolean          active;

      gtk_tree_model_get (tree_view->model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = GIMP_TOOL_ITEM (renderer->viewable);

      g_object_get (toggle,
                    "active", &active,
                    NULL);

      gimp_tool_item_set_visible (tool_item, ! active);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}

static GimpToolItem *
gimp_tool_editor_get_selected_tool_item (GimpToolEditor *tool_editor)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (tool_editor);

  if (tool_editor->priv->container)
    {
      GimpViewRenderer *renderer;
      GimpToolItem     *tool_item;
      GtkTreeSelection *selection;
      GtkTreeModel     *model;
      GtkTreeIter       iter;

      selection = gtk_tree_view_get_selection (tree_view->view);

      if (! gtk_tree_selection_get_selected (selection, &model, &iter))
        return NULL;

      gtk_tree_model_get (model, &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = GIMP_TOOL_ITEM (renderer->viewable);

      g_object_unref (renderer);

      return tool_item;
    }

  return NULL;
}

static GimpContainer *
gimp_tool_editor_get_tool_item_container (GimpToolEditor *tool_editor,
                                          GimpToolItem   *tool_item)
{
  GimpViewable *parent;

  parent = gimp_viewable_get_parent (GIMP_VIEWABLE (tool_item));

  if (parent)
    {
      return gimp_viewable_get_children (parent);
    }
  else
    {
      return tool_editor->priv->container;
    }
}

static void
gimp_tool_editor_update_container (GimpToolEditor *tool_editor)
{
  GimpContainerView *container_view = GIMP_CONTAINER_VIEW (tool_editor);
  GimpContainer     *container;
  GimpContext       *context;

  g_clear_pointer (&tool_editor->priv->tool_item_notify_handler,
                   gimp_tree_handler_disconnect);

  g_clear_pointer (&tool_editor->priv->initial_tool_state, g_free);

  container = gimp_container_view_get_container (container_view);
  context   = gimp_container_view_get_context   (container_view);

  if (container && context)
    {
      GString          *string;
      GimpConfigWriter *writer;

      tool_editor->priv->container = container;
      tool_editor->priv->context   = context;

      tool_editor->priv->tool_item_notify_handler = gimp_tree_handler_connect (
        container, "notify",
        G_CALLBACK (gimp_tool_editor_tool_item_notify),
        tool_editor);

      /* save initial tool order */
      string = g_string_new (NULL);

      writer = gimp_config_writer_new_string (string);

      gimp_tools_serialize (context->gimp, container, writer);

      gimp_config_writer_finish (writer, NULL, NULL);

      tool_editor->priv->initial_tool_state = g_string_free (string, FALSE);
    }
}

static void
gimp_tool_editor_update_sensitivity (GimpToolEditor *tool_editor)
{
  GimpToolItem *tool_item;

  tool_item = gimp_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      GimpContainer *container;
      gint           index;

      container = gimp_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = gimp_container_get_child_index (container,
                                              GIMP_OBJECT (tool_item));

      gtk_widget_set_sensitive (
        tool_editor->priv->new_group_button,
        gimp_viewable_get_parent (GIMP_VIEWABLE (tool_item)) == NULL);

      gtk_widget_set_sensitive (
        tool_editor->priv->raise_button,
        index > 0);

      gtk_widget_set_sensitive (
        tool_editor->priv->lower_button,
        index < gimp_container_get_n_children (container) - 1);

      gtk_widget_set_sensitive (
        tool_editor->priv->delete_button,
        gimp_viewable_get_children (GIMP_VIEWABLE (tool_item)) != NULL);
    }
  else
    {
      gtk_widget_set_sensitive (tool_editor->priv->new_group_button, TRUE);
      gtk_widget_set_sensitive (tool_editor->priv->raise_button,     FALSE);
      gtk_widget_set_sensitive (tool_editor->priv->lower_button,     FALSE);
      gtk_widget_set_sensitive (tool_editor->priv->delete_button,    FALSE);
    }
}


/*  public functions  */

GtkWidget *
gimp_tool_editor_new (GimpContainer *container,
                      GimpContext   *context,
                      gint           view_size,
                      gint           view_border_width)
{
  GimpContainerView *container_view;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  container_view = g_object_new (GIMP_TYPE_TOOL_EDITOR,
                                 "view-size",         view_size,
                                 "view-border-width", view_border_width,
                                 NULL);

  gimp_container_view_set_context     (container_view, context);
  gimp_container_view_set_container   (container_view, container);
  gimp_container_view_set_reorderable (container_view, TRUE);

  return GTK_WIDGET (container_view);
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
  GScanner *scanner;

  g_return_if_fail (GIMP_IS_TOOL_EDITOR (tool_editor));

  scanner = gimp_scanner_new_string (tool_editor->priv->initial_tool_state, -1,
                                     NULL);

  gimp_tools_deserialize (tool_editor->priv->context->gimp,
                          tool_editor->priv->container,
                          scanner);

  gimp_scanner_destroy (scanner);
}
