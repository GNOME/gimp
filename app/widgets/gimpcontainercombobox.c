/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainercombobox.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainercombobox.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"


enum
{
  PROP_0,
  PROP_ELLIPSIZE = GIMP_CONTAINER_VIEW_PROP_LAST + 1
};


static void     gimp_container_combo_box_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_container_combo_box_set_property (GObject                *object,
                                                       guint                   property_id,
                                                       const GValue           *value,
                                                       GParamSpec             *pspec);
static void     gimp_container_combo_box_get_property (GObject                *object,
                                                       guint                   property_id,
                                                       GValue                 *value,
                                                       GParamSpec             *pspec);

static void     gimp_container_combo_box_set_context  (GimpContainerView      *view,
                                                       GimpContext            *context);
static gpointer gimp_container_combo_box_insert_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                parent_insert_data,
                                                       gint                    index);
static void     gimp_container_combo_box_remove_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static void     gimp_container_combo_box_reorder_item (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gint                    new_index,
                                                       gpointer                insert_data);
static void     gimp_container_combo_box_rename_item  (GimpContainerView      *view,
                                                       GimpViewable           *viewable,
                                                       gpointer                insert_data);
static gboolean  gimp_container_combo_box_select_items(GimpContainerView      *view,
                                                       GList                  *viewables,
                                                       GList                  *paths);
static void     gimp_container_combo_box_clear_items  (GimpContainerView      *view);
static void    gimp_container_combo_box_set_view_size (GimpContainerView      *view);
static gint    gimp_container_combo_box_get_selected  (GimpContainerView      *view,
                                                       GList                 **items,
                                                       GList                 **items_data);

static void     gimp_container_combo_box_changed      (GtkComboBox            *combo_box,
                                                       GimpContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (GimpContainerComboBox, gimp_container_combo_box,
                         GTK_TYPE_COMBO_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_container_combo_box_view_iface_init))

#define parent_class gimp_container_combo_box_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_container_combo_box_class_init (GimpContainerComboBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_container_combo_box_set_property;
  object_class->get_property = gimp_container_combo_box_get_property;

  gimp_container_view_install_properties (object_class);

  g_object_class_install_property (object_class,
                                   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize", NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_MIDDLE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_container_combo_box_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (GIMP_TYPE_CONTAINER_VIEW);

  iface->set_context   = gimp_container_combo_box_set_context;
  iface->insert_item   = gimp_container_combo_box_insert_item;
  iface->remove_item   = gimp_container_combo_box_remove_item;
  iface->reorder_item  = gimp_container_combo_box_reorder_item;
  iface->rename_item   = gimp_container_combo_box_rename_item;
  iface->select_items  = gimp_container_combo_box_select_items;
  iface->clear_items   = gimp_container_combo_box_clear_items;
  iface->set_view_size = gimp_container_combo_box_set_view_size;
  iface->get_selected  = gimp_container_combo_box_get_selected;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
gimp_container_combo_box_init (GimpContainerComboBox *combo)
{
  GtkTreeModel    *model;
  GtkCellLayout   *layout;
  GtkCellRenderer *cell;
  GType            types[GIMP_CONTAINER_TREE_STORE_N_COLUMNS];
  gint             n_types = 0;

  gimp_container_tree_store_columns_init (types, &n_types);

  model = gimp_container_tree_store_new (GIMP_CONTAINER_VIEW (combo),
                                         n_types, types);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);

  g_object_unref (model);

  layout = GTK_CELL_LAYOUT (combo);

  cell = gimp_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (layout, cell, FALSE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "renderer",
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  "sensitive",
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                                  NULL);

  gimp_container_tree_store_add_renderer_cell (GIMP_CONTAINER_TREE_STORE (model),
                                               cell, -1);

  combo->viewable_renderer = cell;

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text",
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME,
                                  "sensitive",
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
                                  NULL);

  combo->text_renderer = cell;

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_container_combo_box_changed),
                    combo);

  gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
}

static void
gimp_container_combo_box_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpContainerComboBox *combo = GIMP_CONTAINER_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_object_set_property (G_OBJECT (combo->text_renderer),
                             pspec->name, value);
      break;

    default:
      gimp_container_view_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_container_combo_box_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpContainerComboBox *combo = GIMP_CONTAINER_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_object_get_property (G_OBJECT (combo->text_renderer),
                             pspec->name, value);
      break;

    default:
      gimp_container_view_get_property (object, property_id, value, pspec);
      break;
    }
}

GtkWidget *
gimp_container_combo_box_new (GimpContainer *container,
                              GimpContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  GtkWidget         *combo_box;
  GimpContainerView *view;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  combo_box = g_object_new (GIMP_TYPE_CONTAINER_COMBO_BOX, NULL);

  view = GIMP_CONTAINER_VIEW (combo_box);

  gimp_container_view_set_view_size (view, view_size, view_border_width);

  if (container)
    gimp_container_view_set_container (view, container);

  if (context)
    gimp_container_view_set_context (view, context);

  return combo_box;
}


/*  GimpContainerView methods  */

static void
gimp_container_combo_box_set_context (GimpContainerView *view,
                                      GimpContext       *context)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  parent_view_iface->set_context (view, context);

  if (model)
    gimp_container_tree_store_set_context (GIMP_CONTAINER_TREE_STORE (model),
                                           context);
}

static gpointer
gimp_container_combo_box_insert_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    {
      GtkTreeIter *iter;

      iter = gimp_container_tree_store_insert_item (GIMP_CONTAINER_TREE_STORE (model),
                                                    viewable,
                                                    parent_insert_data,
                                                    index);

      if (gtk_tree_model_iter_n_children (model, NULL) == 1)
        {
          /*  GimpContainerViews don't select items by default  */
          gtk_combo_box_set_active (GTK_COMBO_BOX (view), -1);

          gtk_widget_set_sensitive (GTK_WIDGET (view), TRUE);
        }

      return iter;
    }

  return NULL;
}

static void
gimp_container_combo_box_remove_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    {
      GtkTreeIter *iter = insert_data;

      gimp_container_tree_store_remove_item (GIMP_CONTAINER_TREE_STORE (model),
                                             viewable,
                                             iter);

      if (iter && gtk_tree_model_iter_n_children (model, NULL) == 0)
        {
          gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);
        }
    }
}

static void
gimp_container_combo_box_reorder_item (GimpContainerView *view,
                                       GimpViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    gimp_container_tree_store_reorder_item (GIMP_CONTAINER_TREE_STORE (model),
                                            viewable,
                                            new_index,
                                            insert_data);
}

static void
gimp_container_combo_box_rename_item (GimpContainerView *view,
                                      GimpViewable      *viewable,
                                      gpointer           insert_data)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    gimp_container_tree_store_rename_item (GIMP_CONTAINER_TREE_STORE (model),
                                           viewable,
                                           insert_data);
}

static gboolean
gimp_container_combo_box_select_items (GimpContainerView *view,
                                       GList             *viewables,
                                       GList             *paths)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX (view);

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), FALSE);
  /* Only zero or one items may selected in a GimpContainerComboBox. */
  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  if (gtk_combo_box_get_model (GTK_COMBO_BOX (view)))
    {
      g_signal_handlers_block_by_func (combo_box,
                                       gimp_container_combo_box_changed,
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
              GimpViewRenderer *renderer;

              gtk_tree_model_get (model, &iter,
                                  GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
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
                                         gimp_container_combo_box_changed,
                                         view);
    }

  return TRUE;
}

static void
gimp_container_combo_box_clear_items (GimpContainerView *view)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    gimp_container_tree_store_clear_items (GIMP_CONTAINER_TREE_STORE (model));

  gtk_widget_set_sensitive (GTK_WIDGET (view), FALSE);

  parent_view_iface->clear_items (view);
}

static void
gimp_container_combo_box_set_view_size (GimpContainerView *view)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (view));

  if (model)
    gimp_container_tree_store_set_view_size (GIMP_CONTAINER_TREE_STORE (model));
}

static gint
gimp_container_combo_box_get_selected (GimpContainerView  *view,
                                       GList             **items,
                                       GList             **items_data)
{
  GtkComboBox      *combo_box = GTK_COMBO_BOX (view);
  GimpViewRenderer *renderer  = NULL;
  GtkTreeIter       iter;
  gint              selected  = 0;

  if (gtk_combo_box_get_active_iter (combo_box, &iter))
    gtk_tree_model_get (gtk_combo_box_get_model (combo_box), &iter,
                        GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
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
gimp_container_combo_box_changed (GtkComboBox       *combo,
                                  GimpContainerView *view)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      gimp_container_view_item_selected (view, renderer->viewable);
      g_object_unref (renderer);
    }
}
