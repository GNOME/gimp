/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerentry.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainerentry.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"


static void     gimp_container_entry_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_container_entry_set_context  (GimpContainerView      *view,
                                                   GimpContext            *context);
static gpointer gimp_container_entry_insert_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                parent_insert_data,
                                                   gint                    index);
static void     gimp_container_entry_remove_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static void     gimp_container_entry_reorder_item (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gint                    new_index,
                                                   gpointer                insert_data);
static void     gimp_container_entry_rename_item  (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static gboolean  gimp_container_entry_select_item (GimpContainerView      *view,
                                                   GimpViewable           *viewable,
                                                   gpointer                insert_data);
static void     gimp_container_entry_clear_items  (GimpContainerView      *view);
static void    gimp_container_entry_set_view_size (GimpContainerView      *view);

static void     gimp_container_entry_changed      (GtkEntry               *entry,
                                                   GimpContainerView      *view);
static void   gimp_container_entry_match_selected (GtkEntryCompletion     *widget,
                                                   GtkTreeModel           *model,
                                                   GtkTreeIter            *iter,
                                                   GimpContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (GimpContainerEntry, gimp_container_entry,
                         GTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_entry_view_iface_init))

#define parent_class gimp_container_entry_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_container_entry_class_init (GimpContainerEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_container_view_set_property;
  object_class->get_property = gimp_container_view_get_property;

  gimp_container_view_install_properties (object_class);
}

static void
gimp_container_entry_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (GIMP_TYPE_CONTAINER_VIEW);

  iface->set_context   = gimp_container_entry_set_context;
  iface->insert_item   = gimp_container_entry_insert_item;
  iface->remove_item   = gimp_container_entry_remove_item;
  iface->reorder_item  = gimp_container_entry_reorder_item;
  iface->rename_item   = gimp_container_entry_rename_item;
  iface->select_item   = gimp_container_entry_select_item;
  iface->clear_items   = gimp_container_entry_clear_items;
  iface->set_view_size = gimp_container_entry_set_view_size;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
gimp_container_entry_init (GimpContainerEntry *entry)
{
  GtkEntryCompletion *completion;
  GtkTreeModel       *model;
  GtkCellRenderer    *cell;
  GType               types[GIMP_CONTAINER_TREE_STORE_N_COLUMNS];
  gint                n_types = 0;

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "inline-completion",  TRUE,
                             "popup-single-match", FALSE,
                             "popup-set-width",    FALSE,
                             NULL);

  gimp_container_tree_store_columns_init (types, &n_types);

  model = gimp_container_tree_store_new (GIMP_CONTAINER_VIEW (entry),
                                         n_types, types);
  gimp_container_tree_store_set_use_name (GIMP_CONTAINER_TREE_STORE (model),
                                          TRUE);

  gtk_entry_completion_set_model (completion, model);
  g_object_unref (model);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (gimp_container_entry_match_selected),
                    entry);

  g_object_unref (completion);

  /*  FIXME: This can be done better with GTK+ 2.6.  */

  cell = gimp_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "renderer",
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  gimp_container_tree_store_add_renderer_cell (GIMP_CONTAINER_TREE_STORE (model),
                                               cell);

  gtk_entry_completion_set_text_column (completion,
                                        GIMP_CONTAINER_TREE_STORE_COLUMN_NAME);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_container_entry_changed),
                    entry);
}

GtkWidget *
gimp_container_entry_new (GimpContainer *container,
                          GimpContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  GtkWidget         *entry;
  GimpContainerView *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  entry = g_object_new (GIMP_TYPE_CONTAINER_ENTRY, NULL);

  view = GIMP_CONTAINER_VIEW (entry);

  gimp_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return entry;
}


/*  GimpContainerView methods  */

static GtkTreeModel *
gimp_container_entry_get_model (GimpContainerView *view)
{
  GtkEntryCompletion *completion;

  completion = gtk_entry_get_completion (GTK_ENTRY (view));

  if (completion)
    return gtk_entry_completion_get_model (completion);

  return NULL;
}

static void
gimp_container_entry_set_context (GimpContainerView *view,
                                  GimpContext       *context)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  parent_view_iface->set_context (view, context);

  if (model)
    gimp_container_tree_store_set_context (GIMP_CONTAINER_TREE_STORE (model),
                                           context);
}

static gpointer
gimp_container_entry_insert_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  return gimp_container_tree_store_insert_item (GIMP_CONTAINER_TREE_STORE (model),
                                                viewable,
                                                parent_insert_data,
                                                index);
}

static void
gimp_container_entry_remove_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  gimp_container_tree_store_remove_item (GIMP_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static void
gimp_container_entry_reorder_item (GimpContainerView *view,
                                   GimpViewable      *viewable,
                                   gint               new_index,
                                   gpointer           insert_data)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  gimp_container_tree_store_reorder_item (GIMP_CONTAINER_TREE_STORE (model),
                                          viewable,
                                          new_index,
                                          insert_data);
}

static void
gimp_container_entry_rename_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  gimp_container_tree_store_rename_item (GIMP_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static gboolean
gimp_container_entry_select_item (GimpContainerView *view,
                                  GimpViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkEntry    *entry = GTK_ENTRY (view);
  GtkTreeIter *iter  = insert_data;

  g_signal_handlers_block_by_func (entry,
                                   gimp_container_entry_changed,
                                   view);

  if (iter)
    {
      gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, NULL);
    }
  else
    {
      /* The selected item does not exist. */
      GdkColor     gdk_red;

      gdk_red.red = 65535;
      gdk_red.green = 0;
      gdk_red.blue = 0;

      gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, &gdk_red);
    }
  gtk_entry_set_text (entry, viewable? gimp_object_get_name (viewable) : "");

  g_signal_handlers_unblock_by_func (entry,
                                     gimp_container_entry_changed,
                                     view);

  return TRUE;
}

static void
gimp_container_entry_clear_items (GimpContainerView *view)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  /* model is NULL in dispose() */
  if (model)
    gimp_container_tree_store_clear_items (GIMP_CONTAINER_TREE_STORE (model));

  parent_view_iface->clear_items (view);
}

static void
gimp_container_entry_set_view_size (GimpContainerView *view)
{
  GtkTreeModel *model = gimp_container_entry_get_model (view);

  gimp_container_tree_store_set_view_size (GIMP_CONTAINER_TREE_STORE (model));
}

static void
gimp_container_entry_changed (GtkEntry          *entry,
                              GimpContainerView *view)
{
  GimpContainer *container = gimp_container_view_get_container (view);
  GimpObject    *object;
  const gchar   *text;

  if (! container)
    return;

  text = gtk_entry_get_text (entry);

  object = gimp_container_get_child_by_name (container, text);

  if (object)
    {
      gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, NULL);
      gimp_container_view_item_selected (view, GIMP_VIEWABLE (object));
    }
  else
    {
      /* While editing the entry, contents shows in red for non-existent item. */
      GdkColor     gdk_red;

      gdk_red.red = 65535;
      gdk_red.green = 0;
      gdk_red.blue = 0;

      gtk_widget_modify_text (GTK_WIDGET (entry), GTK_STATE_NORMAL, &gdk_red);
    }
}

static void
gimp_container_entry_match_selected (GtkEntryCompletion *widget,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter,
                                     GimpContainerView  *view)
{
  GimpViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  gimp_container_view_item_selected (view, renderer->viewable);
  g_object_unref (renderer);
}
