/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainertreestore.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "widgets-types.h"

#include "libligmabase/ligmabase.h"

#include "core/ligmacontainer.h"
#include "core/ligmaviewable.h"

#include "ligmacellrendererviewable.h"
#include "ligmacontainertreestore.h"
#include "ligmacontainerview.h"
#include "ligmaviewrenderer.h"


enum
{
  PROP_0,
  PROP_CONTAINER_VIEW,
  PROP_USE_NAME
};


typedef struct _LigmaContainerTreeStorePrivate LigmaContainerTreeStorePrivate;

struct _LigmaContainerTreeStorePrivate
{
  LigmaContainerView *container_view;
  GList             *renderer_cells;
  GList             *renderer_columns;
  gboolean           use_name;
};

#define GET_PRIVATE(store) \
        ((LigmaContainerTreeStorePrivate *) ligma_container_tree_store_get_instance_private ((LigmaContainerTreeStore *) (store)))


static void   ligma_container_tree_store_constructed     (GObject                *object);
static void   ligma_container_tree_store_finalize        (GObject                *object);
static void   ligma_container_tree_store_set_property    (GObject                *object,
                                                         guint                   property_id,
                                                         const GValue           *value,
                                                         GParamSpec             *pspec);
static void   ligma_container_tree_store_get_property    (GObject                *object,
                                                         guint                   property_id,
                                                         GValue                 *value,
                                                         GParamSpec             *pspec);

static void   ligma_container_tree_store_set             (LigmaContainerTreeStore *store,
                                                         GtkTreeIter            *iter,
                                                         LigmaViewable           *viewable);
static void   ligma_container_tree_store_renderer_update (LigmaViewRenderer       *renderer,
                                                         LigmaContainerTreeStore *store);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaContainerTreeStore, ligma_container_tree_store,
                            GTK_TYPE_TREE_STORE)

#define parent_class ligma_container_tree_store_parent_class


static void
ligma_container_tree_store_class_init (LigmaContainerTreeStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_container_tree_store_constructed;
  object_class->finalize     = ligma_container_tree_store_finalize;
  object_class->set_property = ligma_container_tree_store_set_property;
  object_class->get_property = ligma_container_tree_store_get_property;

  g_object_class_install_property (object_class, PROP_CONTAINER_VIEW,
                                   g_param_spec_object ("container-view",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER_VIEW,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USE_NAME,
                                   g_param_spec_boolean ("use-name",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_container_tree_store_init (LigmaContainerTreeStore *store)
{
}

static void
ligma_container_tree_store_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
ligma_container_tree_store_finalize (GObject *object)
{
  LigmaContainerTreeStorePrivate *private = GET_PRIVATE (object);

  g_list_free (private->renderer_cells);
  g_list_free (private->renderer_columns);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_container_tree_store_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaContainerTreeStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CONTAINER_VIEW:
      private->container_view = g_value_get_object (value); /* don't ref */
      break;
    case PROP_USE_NAME:
      private->use_name = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_container_tree_store_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaContainerTreeStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CONTAINER_VIEW:
      g_value_set_object (value, private->container_view);
      break;
    case PROP_USE_NAME:
      g_value_set_boolean (value, private->use_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkTreeModel *
ligma_container_tree_store_new (LigmaContainerView *container_view,
                               gint               n_columns,
                               GType             *types)
{
  LigmaContainerTreeStore *store;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_VIEW (container_view), NULL);
  g_return_val_if_fail (n_columns >= LIGMA_CONTAINER_TREE_STORE_N_COLUMNS, NULL);
  g_return_val_if_fail (types != NULL, NULL);

  store = g_object_new (LIGMA_TYPE_CONTAINER_TREE_STORE,
                        "container-view", container_view,
                        NULL);

  gtk_tree_store_set_column_types (GTK_TREE_STORE (store), n_columns, types);

  return GTK_TREE_MODEL (store);
}

void
ligma_container_tree_store_add_renderer_cell (LigmaContainerTreeStore *store,
                                             GtkCellRenderer        *cell,
                                             gint                    column_number)
{
  LigmaContainerTreeStorePrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));
  g_return_if_fail (LIGMA_IS_CELL_RENDERER_VIEWABLE (cell));

  private = GET_PRIVATE (store);

  private->renderer_cells = g_list_prepend (private->renderer_cells, cell);
  if (column_number >= 0)
    private->renderer_columns = g_list_append (private->renderer_columns,
                                               GINT_TO_POINTER (column_number));

}

LigmaViewRenderer *
ligma_container_tree_store_get_renderer (LigmaContainerTreeStore *store,
                                        GtkTreeIter            *iter)
{
  LigmaContainerTreeStorePrivate *private;
  LigmaViewRenderer              *renderer = NULL;
  GList                         *c;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store), NULL);

  private = GET_PRIVATE (store);

  for (c = private->renderer_columns; c; c = c->next)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                          GPOINTER_TO_INT (c->data), &renderer,
                          -1);

      if (renderer)
        break;
    }

  g_return_val_if_fail (renderer != NULL, NULL);

  return renderer;
}

void
ligma_container_tree_store_set_use_name (LigmaContainerTreeStore *store,
                                        gboolean                use_name)
{
  LigmaContainerTreeStorePrivate *private;

  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  if (private->use_name != use_name)
    {
      private->use_name = use_name ? TRUE : FALSE;
      g_object_notify (G_OBJECT (store), "use-name");
    }
}

gboolean
ligma_container_tree_store_get_use_name (LigmaContainerTreeStore *store)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store), FALSE);

  return GET_PRIVATE (store)->use_name;
}

static gboolean
ligma_container_tree_store_set_context_foreach (GtkTreeModel *model,
                                               GtkTreePath  *path,
                                               GtkTreeIter  *iter,
                                               gpointer      data)
{
  LigmaContext      *context = data;
  LigmaViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  ligma_view_renderer_set_context (renderer, context);

  g_object_unref (renderer);

  return FALSE;
}

void
ligma_container_tree_store_set_context (LigmaContainerTreeStore *store,
                                       LigmaContext            *context)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));

  gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                          ligma_container_tree_store_set_context_foreach,
                          context);
}

GtkTreeIter *
ligma_container_tree_store_insert_item (LigmaContainerTreeStore *store,
                                       LigmaViewable           *viewable,
                                       GtkTreeIter            *parent,
                                       gint                    index)
{
  GtkTreeIter iter;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store), NULL);

  if (index == -1)
    gtk_tree_store_append (GTK_TREE_STORE (store), &iter, parent);
  else
    gtk_tree_store_insert (GTK_TREE_STORE (store), &iter, parent, index);

  ligma_container_tree_store_set (store, &iter, viewable);

  return gtk_tree_iter_copy (&iter);
}

void
ligma_container_tree_store_remove_item (LigmaContainerTreeStore *store,
                                       LigmaViewable           *viewable,
                                       GtkTreeIter            *iter)
{
  if (iter)
    {
      GtkTreeModel *model = GTK_TREE_MODEL (store);
      GtkTreePath  *path;

      /* emit a "row-changed" signal for 'iter', so that editing of
       * corresponding tree-view rows is canceled.  otherwise, if we remove the
       * item while a corresponding row is being edited, bad things happen (see
       * bug #792991).
       */
      path = gtk_tree_model_get_path (model, iter);
      gtk_tree_model_row_changed (model, path, iter);
      gtk_tree_path_free (path);

      gtk_tree_store_remove (GTK_TREE_STORE (store), iter);

      /*  If the store is empty after this remove, clear out renderers
       *  from all cells so they don't keep refing the viewables
       *  (see bug #149906).
       */
      if (! gtk_tree_model_iter_n_children (model, NULL))
        {
          LigmaContainerTreeStorePrivate *private = GET_PRIVATE (store);
          GList                         *list;

          for (list = private->renderer_cells; list; list = list->next)
            g_object_set (list->data, "renderer", NULL, NULL);
        }
    }
}

void
ligma_container_tree_store_reorder_item (LigmaContainerTreeStore *store,
                                        LigmaViewable           *viewable,
                                        gint                    new_index,
                                        GtkTreeIter            *iter)
{
  LigmaContainerTreeStorePrivate *private;
  LigmaViewable                  *parent;
  LigmaContainer                 *container;

  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  if (! iter)
    return;

  parent = ligma_viewable_get_parent (viewable);

  if (parent)
    container = ligma_viewable_get_children (parent);
  else
    container = ligma_container_view_get_container (private->container_view);

  if (new_index == -1 ||
      new_index == ligma_container_get_n_children (container) - 1)
    {
      gtk_tree_store_move_before (GTK_TREE_STORE (store), iter, NULL);
    }
  else if (new_index == 0)
    {
      gtk_tree_store_move_after (GTK_TREE_STORE (store), iter, NULL);
    }
  else
    {
      GtkTreePath *path;
      GtkTreeIter  place_iter;
      gint         depth;
      gint        *indices;
      gint         old_index;

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
      indices = gtk_tree_path_get_indices (path);

      depth = gtk_tree_path_get_depth (path);

      old_index = indices[depth - 1];

      if (new_index != old_index)
        {
          indices[depth - 1] = new_index;

          gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &place_iter, path);

          if (new_index > old_index)
            gtk_tree_store_move_after (GTK_TREE_STORE (store),
                                       iter, &place_iter);
          else
            gtk_tree_store_move_before (GTK_TREE_STORE (store),
                                        iter, &place_iter);
        }

      gtk_tree_path_free (path);
    }
}

gboolean
ligma_container_tree_store_rename_item (LigmaContainerTreeStore *store,
                                       LigmaViewable           *viewable,
                                       GtkTreeIter            *iter)
{
  gboolean new_name_shorter = FALSE;

  g_return_val_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store), FALSE);

  if (iter)
    {
      LigmaContainerTreeStorePrivate *private = GET_PRIVATE (store);
      gchar                         *name;
      gchar                         *old_name;

      if (private->use_name)
        name = (gchar *) ligma_object_get_name (viewable);
      else
        name = ligma_viewable_get_description (viewable, NULL);

      gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, &old_name,
                          -1);

      gtk_tree_store_set (GTK_TREE_STORE (store), iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                          -1);

      if (name && old_name && strlen (name) < strlen (old_name))
        new_name_shorter = TRUE;

      if (! private->use_name)
        g_free (name);

      g_free (old_name);
    }

  return new_name_shorter;
}

void
ligma_container_tree_store_clear_items (LigmaContainerTreeStore *store)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));

  gtk_tree_store_clear (GTK_TREE_STORE (store));

  /*  If the store is empty after this remove, clear out renderers
   *  from all cells so they don't keep refing the viewables
   *  (see bug #149906).
   */
  if (! gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL))
    {
      LigmaContainerTreeStorePrivate *private = GET_PRIVATE (store);
      GList                         *list;

      for (list = private->renderer_cells; list; list = list->next)
        g_object_set (list->data, "renderer", NULL, NULL);
    }
}

typedef struct
{
  gint view_size;
  gint border_width;
} SetSizeForeachData;

static gboolean
ligma_container_tree_store_set_view_size_foreach (GtkTreeModel *model,
                                                 GtkTreePath  *path,
                                                 GtkTreeIter  *iter,
                                                 gpointer      data)
{
  SetSizeForeachData *size_data = data;
  LigmaViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  ligma_view_renderer_set_size (renderer,
                               size_data->view_size,
                               size_data->border_width);

  g_object_unref (renderer);

  return FALSE;
}

void
ligma_container_tree_store_set_view_size (LigmaContainerTreeStore *store)
{
  LigmaContainerTreeStorePrivate *private;
  SetSizeForeachData             size_data;

  g_return_if_fail (LIGMA_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  size_data.view_size =
    ligma_container_view_get_view_size (private->container_view,
                                       &size_data.border_width);

  gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                          ligma_container_tree_store_set_view_size_foreach,
                          &size_data);
}


/*  private functions  */

void
ligma_container_tree_store_columns_init (GType *types,
                                        gint  *n_types)
{
  g_return_if_fail (types != NULL);
  g_return_if_fail (n_types != NULL);
  g_return_if_fail (*n_types == 0);

  ligma_assert (LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER ==
               ligma_container_tree_store_columns_add (types, n_types,
                                                      LIGMA_TYPE_VIEW_RENDERER));

  ligma_assert (LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME ==
               ligma_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_STRING));

  ligma_assert (LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES ==
               ligma_container_tree_store_columns_add (types, n_types,
                                                      PANGO_TYPE_ATTR_LIST));

  ligma_assert (LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE ==
               ligma_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_BOOLEAN));

  ligma_assert (LIGMA_CONTAINER_TREE_STORE_COLUMN_USER_DATA ==
               ligma_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_POINTER));
}

gint
ligma_container_tree_store_columns_add (GType *types,
                                       gint  *n_types,
                                       GType  type)
{
  g_return_val_if_fail (types != NULL, 0);
  g_return_val_if_fail (n_types != NULL, 0);
  g_return_val_if_fail (*n_types >= 0, 0);

  types[*n_types] = type;
  (*n_types)++;

  return *n_types - 1;
}

static void
ligma_container_tree_store_set (LigmaContainerTreeStore *store,
                               GtkTreeIter            *iter,
                               LigmaViewable           *viewable)
{
  LigmaContainerTreeStorePrivate *private = GET_PRIVATE (store);
  LigmaContext                   *context;
  LigmaViewRenderer              *renderer;
  gchar                         *name;
  gint                           view_size;
  gint                           border_width;

  context = ligma_container_view_get_context (private->container_view);

  view_size = ligma_container_view_get_view_size (private->container_view,
                                                 &border_width);

  renderer = ligma_view_renderer_new (context,
                                     G_TYPE_FROM_INSTANCE (viewable),
                                     view_size, border_width,
                                     FALSE);
  ligma_view_renderer_set_viewable (renderer, viewable);
  ligma_view_renderer_remove_idle (renderer);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (ligma_container_tree_store_renderer_update),
                    store);

  if (private->use_name)
    name = (gchar *) ligma_object_get_name (viewable);
  else
    name = ligma_viewable_get_description (viewable, NULL);

  gtk_tree_store_set (GTK_TREE_STORE (store), iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER,       renderer,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,           name,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE, TRUE,
                      -1);

  if (! private->use_name)
    g_free (name);

  g_object_unref (renderer);
}

static void
ligma_container_tree_store_renderer_update (LigmaViewRenderer       *renderer,
                                           LigmaContainerTreeStore *store)
{
  LigmaContainerTreeStorePrivate *private = GET_PRIVATE (store);
  GtkTreeIter                   *iter;

  iter = ligma_container_view_lookup (private->container_view,
                                     renderer->viewable);

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
      gtk_tree_model_row_changed (GTK_TREE_MODEL (store), path, iter);
      gtk_tree_path_free (path);
    }
}
