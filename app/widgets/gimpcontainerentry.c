/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerentry.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"

#include "ligmacellrendererviewable.h"
#include "ligmacontainerentry.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmaviewrenderer.h"


static void     ligma_container_entry_view_iface_init (LigmaContainerViewInterface *iface);

static void     ligma_container_entry_finalize     (GObject                *object);

static void     ligma_container_entry_set_context  (LigmaContainerView      *view,
                                                   LigmaContext            *context);
static gpointer ligma_container_entry_insert_item  (LigmaContainerView      *view,
                                                   LigmaViewable           *viewable,
                                                   gpointer                parent_insert_data,
                                                   gint                    index);
static void     ligma_container_entry_remove_item  (LigmaContainerView      *view,
                                                   LigmaViewable           *viewable,
                                                   gpointer                insert_data);
static void     ligma_container_entry_reorder_item (LigmaContainerView      *view,
                                                   LigmaViewable           *viewable,
                                                   gint                    new_index,
                                                   gpointer                insert_data);
static void     ligma_container_entry_rename_item  (LigmaContainerView      *view,
                                                   LigmaViewable           *viewable,
                                                   gpointer                insert_data);
static gboolean  ligma_container_entry_select_items(LigmaContainerView      *view,
                                                   GList                  *items,
                                                   GList                  *paths);
static void     ligma_container_entry_clear_items  (LigmaContainerView      *view);
static void    ligma_container_entry_set_view_size (LigmaContainerView      *view);
static gint     ligma_container_entry_get_selected (LigmaContainerView      *view,
                                                   GList                 **items,
                                                   GList                 **items_data);

static void     ligma_container_entry_changed      (GtkEntry               *entry,
                                                   LigmaContainerView      *view);
static void   ligma_container_entry_match_selected (GtkEntryCompletion     *widget,
                                                   GtkTreeModel           *model,
                                                   GtkTreeIter            *iter,
                                                   LigmaContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (LigmaContainerEntry, ligma_container_entry,
                         GTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_container_entry_view_iface_init))

#define parent_class ligma_container_entry_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_container_entry_class_init (LigmaContainerEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_container_view_set_property;
  object_class->get_property = ligma_container_view_get_property;
  object_class->finalize     = ligma_container_entry_finalize;

  ligma_container_view_install_properties (object_class);
}

static void
ligma_container_entry_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (LIGMA_TYPE_CONTAINER_VIEW);

  iface->set_context   = ligma_container_entry_set_context;
  iface->insert_item   = ligma_container_entry_insert_item;
  iface->remove_item   = ligma_container_entry_remove_item;
  iface->reorder_item  = ligma_container_entry_reorder_item;
  iface->rename_item   = ligma_container_entry_rename_item;
  iface->select_items  = ligma_container_entry_select_items;
  iface->clear_items   = ligma_container_entry_clear_items;
  iface->set_view_size = ligma_container_entry_set_view_size;
  iface->get_selected  = ligma_container_entry_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
ligma_container_entry_init (LigmaContainerEntry *entry)
{
  GtkEntryCompletion *completion;
  GtkTreeModel       *model;
  GtkCellRenderer    *cell;
  GType               types[LIGMA_CONTAINER_TREE_STORE_N_COLUMNS];
  gint                n_types = 0;

  entry->viewable = NULL;

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "inline-completion",  TRUE,
                             "popup-single-match", FALSE,
                             "popup-set-width",    FALSE,
                             NULL);

  ligma_container_tree_store_columns_init (types, &n_types);

  model = ligma_container_tree_store_new (LIGMA_CONTAINER_VIEW (entry),
                                         n_types, types);
  ligma_container_tree_store_set_use_name (LIGMA_CONTAINER_TREE_STORE (model),
                                          TRUE);

  gtk_entry_completion_set_model (completion, model);
  g_object_unref (model);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (ligma_container_entry_match_selected),
                    entry);

  g_object_unref (completion);

  /*  FIXME: This can be done better with GTK+ 2.6.  */

  cell = ligma_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "renderer",
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  ligma_container_tree_store_add_renderer_cell (LIGMA_CONTAINER_TREE_STORE (model),
                                               cell, -1);

  gtk_entry_completion_set_text_column (completion,
                                        LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (ligma_container_entry_changed),
                    entry);
}

GtkWidget *
ligma_container_entry_new (LigmaContainer *container,
                          LigmaContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  GtkWidget         *entry;
  LigmaContainerView *view;

  g_return_val_if_fail (container == NULL || LIGMA_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);

  entry = g_object_new (LIGMA_TYPE_CONTAINER_ENTRY, NULL);

  view = LIGMA_CONTAINER_VIEW (entry);

  ligma_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    ligma_container_view_set_container (view, container);

  if (context)
    ligma_container_view_set_context (view, context);

  return entry;
}


/*  LigmaContainerView methods  */

static GtkTreeModel *
ligma_container_entry_get_model (LigmaContainerView *view)
{
  GtkEntryCompletion *completion;

  completion = gtk_entry_get_completion (GTK_ENTRY (view));

  if (completion)
    return gtk_entry_completion_get_model (completion);

  return NULL;
}

static void
ligma_container_entry_finalize (GObject *object)
{
  LigmaContainerEntry *entry = LIGMA_CONTAINER_ENTRY (object);

  if (entry->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (entry->viewable),
                                    (gpointer) &entry->viewable);
      entry->viewable = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_container_entry_set_context (LigmaContainerView *view,
                                  LigmaContext       *context)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  parent_view_iface->set_context (view, context);

  if (model)
    ligma_container_tree_store_set_context (LIGMA_CONTAINER_TREE_STORE (model),
                                           context);
}

static gpointer
ligma_container_entry_insert_item (LigmaContainerView *view,
                                  LigmaViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  return ligma_container_tree_store_insert_item (LIGMA_CONTAINER_TREE_STORE (model),
                                                viewable,
                                                parent_insert_data,
                                                index);
}

static void
ligma_container_entry_remove_item (LigmaContainerView *view,
                                  LigmaViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  ligma_container_tree_store_remove_item (LIGMA_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static void
ligma_container_entry_reorder_item (LigmaContainerView *view,
                                   LigmaViewable      *viewable,
                                   gint               new_index,
                                   gpointer           insert_data)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  ligma_container_tree_store_reorder_item (LIGMA_CONTAINER_TREE_STORE (model),
                                          viewable,
                                          new_index,
                                          insert_data);
}

static void
ligma_container_entry_rename_item (LigmaContainerView *view,
                                  LigmaViewable      *viewable,
                                  gpointer           insert_data)
{
  LigmaContainerEntry *container_entry = LIGMA_CONTAINER_ENTRY (view);
  GtkEntry           *entry           = GTK_ENTRY (view);
  GtkTreeModel       *model           = ligma_container_entry_get_model (view);

  if (viewable == container_entry->viewable)
    {
      g_signal_handlers_block_by_func (entry,
                                       ligma_container_entry_changed,
                                       view);

      gtk_entry_set_text (entry, ligma_object_get_name (viewable));

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_container_entry_changed,
                                         view);
    }

  ligma_container_tree_store_rename_item (LIGMA_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static gboolean
ligma_container_entry_select_items (LigmaContainerView   *view,
                                   GList               *viewables,
                                   GList               *paths)
{
  LigmaContainerEntry *container_entry = LIGMA_CONTAINER_ENTRY (view);
  GtkEntry           *entry           = GTK_ENTRY (view);
  LigmaViewable       *viewable        = NULL;

  /* XXX Only support 1 selected viewable for now. */
  if (viewables)
    viewable = viewables->data;

  g_signal_handlers_block_by_func (entry,
                                   ligma_container_entry_changed,
                                   view);

  if (container_entry->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (container_entry->viewable),
                                    (gpointer) &container_entry->viewable);
      container_entry->viewable = NULL;
    }

  if (viewable)
    {
      container_entry->viewable = viewable;
      g_object_add_weak_pointer (G_OBJECT (container_entry->viewable),
                                 (gpointer) &container_entry->viewable);

      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         NULL);
    }
  else
    {
      /* The selected item does not exist. */
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         LIGMA_ICON_WILBER_EEK);
    }

  gtk_entry_set_text (entry, viewable? ligma_object_get_name (viewable) : "");

  g_signal_handlers_unblock_by_func (entry,
                                     ligma_container_entry_changed,
                                     view);

  return TRUE;
}

static void
ligma_container_entry_clear_items (LigmaContainerView *view)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  /* model is NULL in dispose() */
  if (model)
    ligma_container_tree_store_clear_items (LIGMA_CONTAINER_TREE_STORE (model));

  parent_view_iface->clear_items (view);
}

static void
ligma_container_entry_set_view_size (LigmaContainerView *view)
{
  GtkTreeModel *model = ligma_container_entry_get_model (view);

  ligma_container_tree_store_set_view_size (LIGMA_CONTAINER_TREE_STORE (model));
}

static gint
ligma_container_entry_get_selected (LigmaContainerView  *view,
                                   GList             **items,
                                   GList             **items_data)
{
  LigmaContainerEntry *container_entry = LIGMA_CONTAINER_ENTRY (view);

  if (items)
    {
      if (container_entry->viewable != NULL)
        *items = g_list_prepend (NULL, container_entry->viewable);
      else
        *items = NULL;
    }

  return container_entry->viewable == NULL ? 0 : 1;
}

static void
ligma_container_entry_changed (GtkEntry          *entry,
                              LigmaContainerView *view)
{
  LigmaContainerEntry *container_entry = LIGMA_CONTAINER_ENTRY (entry);
  LigmaContainer      *container       = ligma_container_view_get_container (view);
  LigmaObject         *object;
  const gchar        *text;

  if (! container)
    return;

  if (container_entry->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (container_entry->viewable),
                                    (gpointer) &container_entry->viewable);
      container_entry->viewable = NULL;
    }

  text = gtk_entry_get_text (entry);

  object = ligma_container_get_child_by_name (container, text);

  if (object)
    {
      container_entry->viewable = LIGMA_VIEWABLE (object);
      g_object_add_weak_pointer (G_OBJECT (container_entry->viewable),
                                 (gpointer) &container_entry->viewable);

      ligma_container_view_item_selected (view, LIGMA_VIEWABLE (object));

      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         NULL);
    }
  else
    {
      /* While editing the entry, show EEK for non-existent item. */
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         LIGMA_ICON_WILBER_EEK);
    }
}

static void
ligma_container_entry_match_selected (GtkEntryCompletion *widget,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter,
                                     LigmaContainerView  *view)
{
  LigmaViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  ligma_container_view_item_selected (view, renderer->viewable);
  g_object_unref (renderer);
}
