/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooleditor.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@ligma.org>
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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmatoolgroup.h"
#include "core/ligmatreehandler.h"

#include "tools/ligma-tools.h"

#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmaviewrenderer.h"
#include "ligmatooleditor.h"
#include "ligmahelp-ids.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


struct _LigmaToolEditorPrivate
{
  LigmaContainer   *container;
  LigmaContext     *context;

  GtkWidget       *scrolled;

  GtkWidget       *new_group_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  LigmaTreeHandler *tool_item_notify_handler;

  /* State of tools at creation of the editor, stored to support
   * reverting changes
   */
  gchar           *initial_tool_state;
};


/*  local function prototypes  */

static void            ligma_tool_editor_view_iface_init           (LigmaContainerViewInterface *iface);

static void            ligma_tool_editor_constructed               (GObject                    *object);

static gboolean        ligma_tool_editor_select_items              (LigmaContainerView          *view,
                                                                   GList                      *items,
                                                                   GList                      *paths);
static void            ligma_tool_editor_set_container             (LigmaContainerView          *container_view,
                                                                   LigmaContainer              *container);
static void            ligma_tool_editor_set_context               (LigmaContainerView          *container_view,
                                                                   LigmaContext                *context);

static gboolean        ligma_tool_editor_drop_possible             (LigmaContainerTreeView      *tree_view,
                                                                   LigmaDndType                 src_type,
                                                                   GList                      *src_viewables,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void            ligma_tool_editor_drop_viewables            (LigmaContainerTreeView      *tree_view,
                                                                   GList                      *src_viewables,
                                                                   LigmaViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);

static void            ligma_tool_editor_tool_item_notify         (LigmaToolItem                *tool_item,
                                                                  GParamSpec                  *pspec,
                                                                  LigmaToolEditor              *tool_editor);

static void            ligma_tool_editor_eye_data_func            (GtkTreeViewColumn           *tree_column,
                                                                  GtkCellRenderer             *cell,
                                                                  GtkTreeModel                *tree_model,
                                                                  GtkTreeIter                 *iter,
                                                                  gpointer                     data);
static void            ligma_tool_editor_eye_clicked              (GtkCellRendererToggle       *toggle,
                                                                  gchar                       *path_str,
                                                                  GdkModifierType              state,
                                                                  LigmaToolEditor              *tool_editor);

static void            ligma_tool_editor_new_group_clicked        (GtkButton                   *button,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_raise_clicked            (GtkButton                   *button,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_raise_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_lower_clicked            (GtkButton                   *button,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_lower_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_delete_clicked           (GtkButton                   *button,
                                                                  LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_reset_clicked            (GtkButton                   *button,
                                                                  LigmaToolEditor              *tool_editor);

static LigmaToolItem  * ligma_tool_editor_get_selected_tool_item   (LigmaToolEditor              *tool_editor);
static LigmaContainer * ligma_tool_editor_get_tool_item_container  (LigmaToolEditor              *tool_editor,
                                                                  LigmaToolItem                *tool_item);

static void            ligma_tool_editor_update_container         (LigmaToolEditor              *tool_editor);
static void            ligma_tool_editor_update_sensitivity       (LigmaToolEditor              *tool_editor);


G_DEFINE_TYPE_WITH_CODE (LigmaToolEditor, ligma_tool_editor,
                         LIGMA_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (LigmaToolEditor)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_tool_editor_view_iface_init))

#define parent_class ligma_tool_editor_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


/*  private functions  */

static void
ligma_tool_editor_class_init (LigmaToolEditorClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  LigmaContainerTreeViewClass *tree_view_class = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);

  object_class->constructed       = ligma_tool_editor_constructed;

  tree_view_class->drop_possible  = ligma_tool_editor_drop_possible;
  tree_view_class->drop_viewables = ligma_tool_editor_drop_viewables;
}

static void
ligma_tool_editor_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (LIGMA_TYPE_CONTAINER_VIEW);

  iface->select_items  = ligma_tool_editor_select_items;
  iface->set_container = ligma_tool_editor_set_container;
  iface->set_context   = ligma_tool_editor_set_context;
}

static void
ligma_tool_editor_init (LigmaToolEditor *tool_editor)
{
  tool_editor->priv = ligma_tool_editor_get_instance_private (tool_editor);
}

static void
ligma_tool_editor_constructed (GObject *object)
{
  LigmaToolEditor        *tool_editor    = LIGMA_TOOL_EDITOR (object);
  LigmaContainerTreeView *tree_view      = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaContainerView     *container_view = LIGMA_CONTAINER_VIEW (object);
  gint                   view_size;
  gint                   border_width;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view_size = ligma_container_view_get_view_size (container_view,
                                                 &border_width);

  ligma_editor_set_show_name (LIGMA_EDITOR (tool_editor), FALSE);

  gtk_tree_view_set_level_indentation (tree_view->view,
                                       0.8 * (view_size + 2 * border_width));

  ligma_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view),
                              LIGMA_TYPE_TOOL_ITEM,
                              NULL, NULL);

  /* construct tree view */
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer   *eye_cell;
    GtkStyleContext   *tree_style;
    GtkBorder          border;
    gint               icon_size;

    tree_style = gtk_widget_get_style_context (GTK_WIDGET (tool_editor));

    gtk_style_context_get_border (tree_style, 0, &border);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_insert_column (tree_view->view, column, 0);

    eye_cell = ligma_cell_renderer_toggle_new (LIGMA_ICON_VISIBLE);

    g_object_get (eye_cell, "icon-size", &icon_size, NULL);
    icon_size = MIN (icon_size, MAX (view_size - (border.left + border.right),
                                     view_size - (border.top + border.bottom)));
    g_object_set (eye_cell, "icon-size", icon_size, NULL);

    gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
    gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                              ligma_tool_editor_eye_data_func,
                                              tree_view, NULL);

    ligma_container_tree_view_add_toggle_cell (tree_view, eye_cell);

    g_signal_connect (eye_cell, "clicked",
                      G_CALLBACK (ligma_tool_editor_eye_clicked),
                      tool_editor);
  }

  /* buttons */
  tool_editor->priv->new_group_button =
    ligma_editor_add_button (LIGMA_EDITOR (tool_editor), LIGMA_ICON_FOLDER_NEW,
                            _("Create a new tool group"), NULL,
                            G_CALLBACK (ligma_tool_editor_new_group_clicked),
                            NULL,
                            tool_editor);

  tool_editor->priv->raise_button =
    ligma_editor_add_button (LIGMA_EDITOR (tool_editor), LIGMA_ICON_GO_UP,
                            _("Raise this item"),
                            _("Raise this item to the top"),
                            G_CALLBACK (ligma_tool_editor_raise_clicked),
                            G_CALLBACK (ligma_tool_editor_raise_extend_clicked),
                            tool_editor);

  tool_editor->priv->lower_button =
    ligma_editor_add_button (LIGMA_EDITOR (tool_editor), LIGMA_ICON_GO_DOWN,
                            _("Lower this item"),
                            _("Lower this item to the bottom"),
                            G_CALLBACK (ligma_tool_editor_lower_clicked),
                            G_CALLBACK (ligma_tool_editor_lower_extend_clicked),
                            tool_editor);

  tool_editor->priv->delete_button =
    ligma_editor_add_button (LIGMA_EDITOR (tool_editor), LIGMA_ICON_EDIT_DELETE,
                            _("Delete this tool group"), NULL,
                            G_CALLBACK (ligma_tool_editor_delete_clicked),
                            NULL,
                            tool_editor);

  tool_editor->priv->reset_button =
    ligma_editor_add_button (LIGMA_EDITOR (tool_editor), LIGMA_ICON_RESET,
                            _("Reset tool order and visibility"), NULL,
                            G_CALLBACK (ligma_tool_editor_reset_clicked),
                            NULL,
                            tool_editor);

  ligma_tool_editor_update_sensitivity (tool_editor);
}

static gboolean
ligma_tool_editor_select_items (LigmaContainerView   *container_view,
                               GList               *viewables,
                               GList               *paths)
{
  LigmaToolEditor *tool_editor = LIGMA_TOOL_EDITOR (container_view);
  gboolean        result;

  result = parent_view_iface->select_items (container_view,
                                            viewables, paths);

  ligma_tool_editor_update_sensitivity (tool_editor);

  return result;
}

static void
ligma_tool_editor_set_container (LigmaContainerView *container_view,
                                LigmaContainer     *container)
{
  LigmaToolEditor *tool_editor = LIGMA_TOOL_EDITOR (container_view);

  parent_view_iface->set_container (container_view, container);

  ligma_tool_editor_update_container (tool_editor);
}

static void
ligma_tool_editor_set_context (LigmaContainerView *container_view,
                              LigmaContext       *context)
{
  LigmaToolEditor *tool_editor = LIGMA_TOOL_EDITOR (container_view);

  parent_view_iface->set_context (container_view, context);

  ligma_tool_editor_update_container (tool_editor);
}

static gboolean
ligma_tool_editor_drop_possible (LigmaContainerTreeView   *tree_view,
                                LigmaDndType              src_type,
                                GList                   *src_viewables,
                                LigmaViewable            *dest_viewable,
                                GtkTreePath             *drop_path,
                                GtkTreeViewDropPosition  drop_pos,
                                GtkTreeViewDropPosition *return_drop_pos,
                                GdkDragAction           *return_drag_action)
{
  if (LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (
        tree_view,
        src_type, src_viewables, dest_viewable, drop_path, drop_pos,
        return_drop_pos, return_drag_action))
    {
      if (ligma_viewable_get_parent (dest_viewable)       ||
          (ligma_viewable_get_children (dest_viewable)    &&
           (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
            drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
        {
          GList *iter;

          for (iter = src_viewables; iter; iter = iter->next)
            {
              LigmaViewable *src_viewable = iter->data;

              if (ligma_viewable_get_children (src_viewable))
                return FALSE;
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
ligma_tool_editor_drop_viewables (LigmaContainerTreeView   *tree_view,
                                 GList                   *src_viewables,
                                 LigmaViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (tree_view);

  LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (tree_view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);

  if (src_viewables)
    ligma_container_view_select_items (container_view, src_viewables);
}

static void
ligma_tool_editor_new_group_clicked (GtkButton      *button,
                                    LigmaToolEditor *tool_editor)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (tool_editor);
  LigmaContainer     *container;
  LigmaToolItem      *tool_item;
  LigmaToolGroup     *group;
  gint               index = 0;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      if (ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_item)) != NULL)
        return;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_child_index (container,
                                              LIGMA_OBJECT (tool_item));
    }
  else
    {
      container = tool_editor->priv->container;
    }

  if (container)
    {
      group = ligma_tool_group_new ();

      ligma_container_insert (container, LIGMA_OBJECT (group), index);

      g_object_unref (group);

      ligma_container_view_select_item (container_view, LIGMA_VIEWABLE (group));
    }
}

static void
ligma_tool_editor_raise_clicked (GtkButton      *button,
                                LigmaToolEditor *tool_editor)
{
  LigmaToolItem *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      LigmaContainer *container;
      gint           index;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_child_index (container,
                                              LIGMA_OBJECT (tool_item));

      if (index > 0)
        {
          ligma_container_reorder (container,
                                  LIGMA_OBJECT (tool_item), index - 1);
        }
    }
}

static void
ligma_tool_editor_raise_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       LigmaToolEditor  *tool_editor)
{
  LigmaToolItem *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      LigmaContainer *container;
      gint           index;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_child_index (container,
                                              LIGMA_OBJECT (tool_item));

      if (index > 0)
        {
          ligma_container_reorder (container,
                                  LIGMA_OBJECT (tool_item), 0);
        }
    }
}

static void
ligma_tool_editor_lower_clicked (GtkButton      *button,
                                LigmaToolEditor *tool_editor)
{
  LigmaToolItem *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      LigmaContainer *container;
      gint           index;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_child_index (container,
                                              LIGMA_OBJECT (tool_item));

      if (index + 1 < ligma_container_get_n_children (container))
        {
          ligma_container_reorder (container,
                                  LIGMA_OBJECT (tool_item), index + 1);
        }
    }
}

static void
ligma_tool_editor_lower_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       LigmaToolEditor  *tool_editor)
{
  LigmaToolItem *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      LigmaContainer *container;
      gint           index;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_n_children (container) - 1;
      index = MAX (index, 0);

      ligma_container_reorder (container,
                              LIGMA_OBJECT (tool_item), index);
    }
}

static void
ligma_tool_editor_delete_clicked (GtkButton      *button,
                                 LigmaToolEditor *tool_editor)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (tool_editor);
  LigmaToolItem     *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      LigmaContainer *src_container;
      LigmaContainer *dest_container;
      gint           index;
      gint           dest_index;

      src_container  = ligma_viewable_get_children (LIGMA_VIEWABLE (tool_item));
      dest_container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                                 tool_item);

      if (! src_container)
        return;

      index      = ligma_container_get_child_index (dest_container,
                                                   LIGMA_OBJECT (tool_item));
      dest_index = index;

      g_object_ref (tool_item);

      ligma_container_freeze (src_container);
      ligma_container_freeze (dest_container);

      ligma_container_remove (dest_container, LIGMA_OBJECT (tool_item));

      while (! ligma_container_is_empty (src_container))
        {
          LigmaObject *object = ligma_container_get_first_child (src_container);

          g_object_ref (object);

          ligma_container_remove (src_container,  object);
          ligma_container_insert (dest_container, object, dest_index++);

          g_object_unref (object);
        }

      ligma_container_thaw (dest_container);
      ligma_container_thaw (src_container);

      ligma_container_view_select_item (
        container_view,
        LIGMA_VIEWABLE (ligma_container_get_child_by_index (dest_container,
                                                          index)));

      g_object_unref (tool_item);
    }
}

static void
ligma_tool_editor_reset_clicked (GtkButton      *button,
                                LigmaToolEditor *tool_editor)
{
  ligma_tools_reset (tool_editor->priv->context->ligma,
                    tool_editor->priv->container,
                    FALSE);
}

static void
ligma_tool_editor_tool_item_notify (LigmaToolItem   *tool_item,
                                   GParamSpec     *pspec,
                                   LigmaToolEditor *tool_editor)
{
  LigmaContainerTreeView *tree_view      = LIGMA_CONTAINER_TREE_VIEW (tool_editor);
  LigmaContainerView     *container_view = LIGMA_CONTAINER_VIEW (tool_editor);
  GtkTreeIter           *iter;

  iter = ligma_container_view_lookup (container_view,
                                     LIGMA_VIEWABLE (tool_item));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
ligma_tool_editor_eye_data_func (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer   *cell,
                                GtkTreeModel      *tree_model,
                                GtkTreeIter       *iter,
                                gpointer           data)
{
  LigmaViewRenderer *renderer;
  LigmaToolItem     *tool_item;

  gtk_tree_model_get (tree_model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  tool_item = LIGMA_TOOL_ITEM (renderer->viewable);

  g_object_set (cell,
                "active",       ligma_tool_item_get_visible (tool_item),
                "inconsistent", ligma_tool_item_get_visible (tool_item) &&
                                ! ligma_tool_item_get_shown (tool_item),
                NULL);

  g_object_unref (renderer);
}

static void
ligma_tool_editor_eye_clicked (GtkCellRendererToggle *toggle,
                              gchar                 *path_str,
                              GdkModifierType        state,
                              LigmaToolEditor        *tool_editor)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (tool_editor);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaToolItem     *tool_item;
      gboolean          active;

      gtk_tree_model_get (tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = LIGMA_TOOL_ITEM (renderer->viewable);

      g_object_get (toggle,
                    "active", &active,
                    NULL);

      ligma_tool_item_set_visible (tool_item, ! active);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}

static LigmaToolItem *
ligma_tool_editor_get_selected_tool_item (LigmaToolEditor *tool_editor)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (tool_editor);

  if (tool_editor->priv->container)
    {
      LigmaViewRenderer *renderer;
      LigmaToolItem     *tool_item;
      GtkTreeSelection *selection;
      GtkTreeModel     *model;
      GtkTreeIter       iter;

      selection = gtk_tree_view_get_selection (tree_view->view);

      if (! gtk_tree_selection_get_selected (selection, &model, &iter))
        return NULL;

      gtk_tree_model_get (model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = LIGMA_TOOL_ITEM (renderer->viewable);

      g_object_unref (renderer);

      return tool_item;
    }

  return NULL;
}

static LigmaContainer *
ligma_tool_editor_get_tool_item_container (LigmaToolEditor *tool_editor,
                                          LigmaToolItem   *tool_item)
{
  LigmaViewable *parent;

  parent = ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_item));

  if (parent)
    {
      return ligma_viewable_get_children (parent);
    }
  else
    {
      return tool_editor->priv->container;
    }
}

static void
ligma_tool_editor_update_container (LigmaToolEditor *tool_editor)
{
  LigmaContainerView *container_view = LIGMA_CONTAINER_VIEW (tool_editor);
  LigmaContainer     *container;
  LigmaContext       *context;

  g_clear_pointer (&tool_editor->priv->tool_item_notify_handler,
                   ligma_tree_handler_disconnect);

  g_clear_pointer (&tool_editor->priv->initial_tool_state, g_free);

  container = ligma_container_view_get_container (container_view);
  context   = ligma_container_view_get_context   (container_view);

  if (container && context)
    {
      GString          *string;
      LigmaConfigWriter *writer;

      tool_editor->priv->container = container;
      tool_editor->priv->context   = context;

      tool_editor->priv->tool_item_notify_handler = ligma_tree_handler_connect (
        container, "notify",
        G_CALLBACK (ligma_tool_editor_tool_item_notify),
        tool_editor);

      /* save initial tool order */
      string = g_string_new (NULL);

      writer = ligma_config_writer_new_from_string (string);

      ligma_tools_serialize (context->ligma, container, writer);

      ligma_config_writer_finish (writer, NULL, NULL);

      tool_editor->priv->initial_tool_state = g_string_free (string, FALSE);
    }
}

static void
ligma_tool_editor_update_sensitivity (LigmaToolEditor *tool_editor)
{
  LigmaToolItem *tool_item;

  tool_item = ligma_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      LigmaContainer *container;
      gint           index;

      container = ligma_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = ligma_container_get_child_index (container,
                                              LIGMA_OBJECT (tool_item));

      gtk_widget_set_sensitive (
        tool_editor->priv->new_group_button,
        ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_item)) == NULL);

      gtk_widget_set_sensitive (
        tool_editor->priv->raise_button,
        index > 0);

      gtk_widget_set_sensitive (
        tool_editor->priv->lower_button,
        index < ligma_container_get_n_children (container) - 1);

      gtk_widget_set_sensitive (
        tool_editor->priv->delete_button,
        ligma_viewable_get_children (LIGMA_VIEWABLE (tool_item)) != NULL);
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
ligma_tool_editor_new (LigmaContainer *container,
                      LigmaContext   *context,
                      gint           view_size,
                      gint           view_border_width)
{
  LigmaContainerView *container_view;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  container_view = g_object_new (LIGMA_TYPE_TOOL_EDITOR,
                                 "view-size",         view_size,
                                 "view-border-width", view_border_width,
                                 NULL);

  ligma_container_view_set_context     (container_view, context);
  ligma_container_view_set_container   (container_view, container);
  ligma_container_view_set_reorderable (container_view, TRUE);

  return GTK_WIDGET (container_view);
}

/**
 * ligma_tool_editor_revert_changes:
 * @tool_editor:
 *
 * Reverts the tool order and visibility to the state at creation.
 **/
void
ligma_tool_editor_revert_changes (LigmaToolEditor *tool_editor)
{
  GScanner *scanner;

  g_return_if_fail (LIGMA_IS_TOOL_EDITOR (tool_editor));

  scanner = ligma_scanner_new_string (tool_editor->priv->initial_tool_state, -1,
                                     NULL);

  ligma_tools_deserialize (tool_editor->priv->context->ligma,
                          tool_editor->priv->container,
                          scanner);

  ligma_scanner_unref (scanner);
}
