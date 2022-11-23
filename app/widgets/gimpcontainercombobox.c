/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainercombobox.c
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

#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"

#include "ligmacellrendererviewable.h"
#include "ligmacontainercombobox.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmaviewrenderer.h"


enum
{
  PROP_0,
  PROP_ELLIPSIZE = LIGMA_CONTAINER_VIEW_PROP_LAST + 1
};


static void     ligma_container_combo_box_view_iface_init (LigmaContainerViewInterface *iface);

static void     ligma_container_combo_box_set_property (GObject                *object,
                                                       guint                   property_id,
                                                       const GValue           *value,
                                                       GParamSpec             *pspec);
static void     ligma_container_combo_box_get_property (GObject                *object,
                                                       guint                   property_id,
                                                       GValue                 *value,
                                                       GParamSpec             *pspec);

static void     ligma_container_combo_box_set_context  (LigmaContainerView      *view,
                                                       LigmaContext            *context);
static gpointer ligma_container_combo_box_insert_item  (LigmaContainerView      *view,
                                                       LigmaViewable           *viewable,
                                                       gpointer                parent_insert_data,
                                                       gint                    index);
static void     ligma_container_combo_box_remove_item  (LigmaContainerView      *view,
                                                       LigmaViewable           *viewable,
                                                       gpointer                insert_data);
static void     ligma_container_combo_box_reorder_item (LigmaContainerView      *view,
                                                       LigmaViewable           *viewable,
                                                       gint                    new_index,
                                                       gpointer                insert_data);
static void     ligma_container_combo_box_rename_item  (LigmaContainerView      *view,
                                                       LigmaViewable           *viewable,
                                                       gpointer                insert_data);
static gboolean  ligma_container_combo_box_select_items(LigmaContainerView      *view,
                                                       GList                  *viewables,
                                                       GList                  *paths);
static void     ligma_container_combo_box_clear_items  (LigmaContainerView      *view);
static void    ligma_container_combo_box_set_view_size (LigmaContainerView      *view);
static gint    ligma_container_combo_box_get_selected  (LigmaContainerView      *view,
                                                       GList                 **items,
                                                       GList                 **items_data);

static void     ligma_container_combo_box_changed      (GtkComboBox            *combo_box,
                                                       LigmaContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (LigmaContainerComboBox, ligma_container_combo_box,
                         GTK_TYPE_COMBO_BOX,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_container_combo_box_view_iface_init))

#define parent_class ligma_container_combo_box_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_container_combo_box_class_init (LigmaContainerComboBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_container_combo_box_set_property;
  object_class->get_property = ligma_container_combo_box_get_property;

  ligma_container_view_install_properties (object_class);

  g_object_class_install_property (object_class,
                                   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize", NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_MIDDLE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
ligma_container_combo_box_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (LIGMA_TYPE_CONTAINER_VIEW);

  iface->set_context   = ligma_container_combo_box_set_context;
  iface->insert_item   = ligma_container_combo_box_insert_item;
  iface->remove_item   = ligma_container_combo_box_remove_item;
  iface->reorder_item  = ligma_container_combo_box_reorder_item;
  iface->rename_item   = ligma_container_combo_box_rename_item;
  iface->select_items  = ligma_container_combo_box_select_items;
  iface->clear_items   = ligma_container_combo_box_clear_items;
  iface->set_view_size = ligma_container_combo_box_set_view_size;
  iface->get_selected  = ligma_container_combo_box_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
ligma_container_combo_box_init (LigmaContainerComboBox *combo)
{
  GtkTreeModel    *model;
  GtkCellLayout   *layout;
  GtkCellRenderer *cell;
  GType            types[LIGMA_CONTAINER_TREE_STORE_N_COLUMNS];
  gint             n_types = 0;

  ligma_container_tree_store_columns_init (types, &n_types);

  model = ligma_container_tree_store_new (LIGMA_CONTAINER_VIEW (combo),
                                         n_types, types);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);

  g_object_unref (model);

  layout = GTK_CELL_LAYOUT (combo);

  cell = ligma_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (layout, cell, FALSE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "renderer",
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  "sensitive",
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                                  NULL);

  ligma_container_tree_store_add_renderer_cell (LIGMA_CONTAINER_TREE_STORE (model),
                                               cell, -1);

  combo->viewable_renderer = cell;

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text",
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,
                                  "sensitive",
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                                  NULL);

  combo->text_renderer = cell;

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_container_combo_box_changed),
                    combo);

  gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
}

static void
ligma_container_combo_box_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaContainerComboBox *combo = LIGMA_CONTAINER_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_object_set_property (G_OBJECT (combo->text_renderer),
                             pspec->name, value);
      break;

    default:
      ligma_container_view_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
ligma_container_combo_box_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaContainerComboBox *combo = LIGMA_CONTAINER_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_object_get_property (G_OBJECT (combo->text_renderer),
                             pspec->name, value);
      break;

    default:
      ligma_container_view_get_property (object, property_id, value, pspec);
      break;
    }
}

GtkWidget *
ligma_container_combo_box_new (LigmaContainer *container,
                              LigmaContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  GtkWidget         *combo_box;
  LigmaContainerView *view;

  g_return_val_if_fail (container == NULL || LIGMA_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);

  combo_box = g_object_new (LIGMA_TYPE_CONTAINER_COMBO_BOX, NULL);

  view = LIGMA_CONTAINER_VIEW (combo_box);

  ligma_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    ligma_container_view_set_container (view, container);

  if (context)
    ligma_container_view_set_context (view, context);

  return combo_box;
}


/*  LigmaContainerView methods  */

static void
ligma_container_combo_box_set_context (LigmaContainerView *view,
                                      LigmaContext       *context)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  parent_view_iface->set_context (view, context);

  if (model)
    ligma_container_tree_store_set_context (LIGMA_CONTAINER_TREE_STORE (model),
                                           context);
}

static gpointer
ligma_container_combo_box_insert_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    {
      GtkTreeIter *iter;

      iter = ligma_container_tree_store_insert_item (LIGMA_CONTAINER_TREE_STORE (model),
                                                    viewable,
                                                    parent_insert_data,
                                                    index);

      if (gtk_tree_model_iter_n_children (model, NULL) == 1)
        {
          /*  LigmaContainerViews don't select items by default  */
          gtk_combo_box_set_active (GTK_COMBO_BOX (view), -1);

          gtk_widget_set_sensitive (GTK_WIDGET (view), TRUE);
        }

      return iter;
    }

  return NULL;
}

static void
ligma_container_combo_box_remove_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    {
      GtkTreeIter *iter = insert_data;

      ligma_container_tree_store_remove_item (LIGMA_CONTAINER_TREE_STORE (model),
                                             viewable,
                                             iter);

      if (iter && gtk_tree_model_iter_n_children (model, NULL) == 0)
        {
          gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);
        }
    }
}

static void
ligma_container_combo_box_reorder_item (LigmaContainerView *view,
                                       LigmaViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    ligma_container_tree_store_reorder_item (LIGMA_CONTAINER_TREE_STORE (model),
                                            viewable,
                                            new_index,
                                            insert_data);
}

static void
ligma_container_combo_box_rename_item (LigmaContainerView *view,
                                      LigmaViewable      *viewable,
                                      gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    ligma_container_tree_store_rename_item (LIGMA_CONTAINER_TREE_STORE (model),
                                           viewable,
                                           insert_data);
}

static gboolean
ligma_container_combo_box_select_items (LigmaContainerView *view,
                                       GList             *viewables,
                                       GList             *paths)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (view);

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (view), FALSE);
  /* Only zero or one items may selected in a LigmaContainerComboBox. */
  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  if (gtk_combo_box_get_model (GTK_COMBO_BOX (view)))
    {
      g_signal_handlers_block_by_func (combo_box,
                                       ligma_container_combo_box_changed,
                                       view);

      if (viewables)
        {
          GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));
          GtkTreeIter   iter;
          gboolean      iter_valid;

          for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
               iter_valid;
               iter_valid = gtk_tree_model_iter_next (model, &iter))
            {
              LigmaViewRenderer *renderer;

              gtk_tree_model_get (model, &iter,
                                  LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                                  -1);

              if (renderer->viewable == viewables->data)
                {
                  gtk_combo_box_set_active_iter (combo_box, &iter);
                  g_object_unref (renderer);

                  break;
                }
              g_object_unref (renderer);
            }
        }
      else
        {
          gtk_combo_box_set_active (combo_box, -1);
        }

      g_signal_handlers_unblock_by_func (combo_box,
                                         ligma_container_combo_box_changed,
                                         view);
    }

  return TRUE;
}

static void
ligma_container_combo_box_clear_items (LigmaContainerView *view)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    ligma_container_tree_store_clear_items (LIGMA_CONTAINER_TREE_STORE (model));

  gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);

  parent_view_iface->clear_items (view);
}

static void
ligma_container_combo_box_set_view_size (LigmaContainerView *view)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    ligma_container_tree_store_set_view_size (LIGMA_CONTAINER_TREE_STORE (model));
}

static gint
ligma_container_combo_box_get_selected (LigmaContainerView  *view,
                                       GList             **items,
                                       GList             **items_data)
{
  GtkComboBox      *combo_box = GTK_COMBO_BOX (view);
  LigmaViewRenderer *renderer  = NULL;
  GtkTreeIter       iter;
  gint              selected  = 0;

  if (gtk_combo_box_get_active_iter (combo_box, &iter))
    gtk_tree_model_get (gtk_combo_box_get_model (combo_box), &iter,
                        LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                        -1);

  if (items)
    {
      if (renderer != NULL && renderer->viewable != NULL)
        {
          *items   = g_list_prepend (NULL, renderer->viewable);
          selected = 1;
        }
      else
        {
          *items = NULL;
        }
    }
  g_clear_object (&renderer);

  return selected;
}

static void
ligma_container_combo_box_changed (GtkComboBox       *combo,
                                  LigmaContainerView *view)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      LigmaViewRenderer *renderer;

      gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      ligma_container_view_item_selected (view, renderer->viewable);
      g_object_unref (renderer);
    }
}
