/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainertreestore.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcellrendererviewable.h"
#include "gimpcontainertreestore.h"
#include "gimpcontainerview.h"
#include "gimpviewrenderer.h"


enum
{
  PROP_0,
  PROP_CONTAINER_VIEW,
  PROP_USE_NAME
};


typedef struct _GimpContainerTreeStorePrivate GimpContainerTreeStorePrivate;

struct _GimpContainerTreeStorePrivate
{
  GimpContainerView *container_view;
  GList             *renderer_cells;
  GList             *renderer_columns;
  gboolean           use_name;
};

#define GET_PRIVATE(store) \
        ((GimpContainerTreeStorePrivate *) gimp_container_tree_store_get_instance_private ((GimpContainerTreeStore *) (store)))


static void   gimp_container_tree_store_constructed     (GObject                *object);
static void   gimp_container_tree_store_finalize        (GObject                *object);
static void   gimp_container_tree_store_set_property    (GObject                *object,
                                                         guint                   property_id,
                                                         const GValue           *value,
                                                         GParamSpec             *pspec);
static void   gimp_container_tree_store_get_property    (GObject                *object,
                                                         guint                   property_id,
                                                         GValue                 *value,
                                                         GParamSpec             *pspec);

static void   gimp_container_tree_store_set             (GimpContainerTreeStore *store,
                                                         GtkTreeIter            *iter,
                                                         GimpViewable           *viewable);
static void   gimp_container_tree_store_renderer_update (GimpViewRenderer       *renderer,
                                                         GimpContainerTreeStore *store);


G_DEFINE_TYPE_WITH_PRIVATE (GimpContainerTreeStore, gimp_container_tree_store,
                            GTK_TYPE_TREE_STORE)

#define parent_class gimp_container_tree_store_parent_class


static void
gimp_container_tree_store_class_init (GimpContainerTreeStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_container_tree_store_constructed;
  object_class->finalize     = gimp_container_tree_store_finalize;
  object_class->set_property = gimp_container_tree_store_set_property;
  object_class->get_property = gimp_container_tree_store_get_property;

  g_object_class_install_property (object_class, PROP_CONTAINER_VIEW,
                                   g_param_spec_object ("container-view",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER_VIEW,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USE_NAME,
                                   g_param_spec_boolean ("use-name",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_container_tree_store_init (GimpContainerTreeStore *store)
{
}

static void
gimp_container_tree_store_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_container_tree_store_finalize (GObject *object)
{
  GimpContainerTreeStorePrivate *private = GET_PRIVATE (object);

  g_list_free (private->renderer_cells);
  g_list_free (private->renderer_columns);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_container_tree_store_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpContainerTreeStorePrivate *private = GET_PRIVATE (object);

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
gimp_container_tree_store_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpContainerTreeStorePrivate *private = GET_PRIVATE (object);

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
gimp_container_tree_store_new (GimpContainerView *container_view,
                               gint               n_columns,
                               GType             *types)
{
  GimpContainerTreeStore *store;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (container_view), NULL);
  g_return_val_if_fail (n_columns >= GIMP_CONTAINER_TREE_STORE_N_COLUMNS, NULL);
  g_return_val_if_fail (types != NULL, NULL);

  store = g_object_new (GIMP_TYPE_CONTAINER_TREE_STORE,
                        "container-view", container_view,
                        NULL);

  gtk_tree_store_set_column_types (GTK_TREE_STORE (store), n_columns, types);

  return GTK_TREE_MODEL (store);
}

void
gimp_container_tree_store_add_renderer_cell (GimpContainerTreeStore *store,
                                             GtkCellRenderer        *cell,
                                             gint                    column_number)
{
  GimpContainerTreeStorePrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));
  g_return_if_fail (GIMP_IS_CELL_RENDERER_VIEWABLE (cell));

  private = GET_PRIVATE (store);

  private->renderer_cells = g_list_prepend (private->renderer_cells, cell);
  if (column_number >= 0)
    private->renderer_columns = g_list_append (private->renderer_columns,
                                               GINT_TO_POINTER (column_number));

}

GimpViewRenderer *
gimp_container_tree_store_get_renderer (GimpContainerTreeStore *store,
                                        GtkTreeIter            *iter)
{
  GimpContainerTreeStorePrivate *private;
  GimpViewRenderer              *renderer = NULL;
  GList                         *c;

  g_return_val_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store), NULL);

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
gimp_container_tree_store_set_use_name (GimpContainerTreeStore *store,
                                        gboolean                use_name)
{
  GimpContainerTreeStorePrivate *private;

  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  if (private->use_name != use_name)
    {
      private->use_name = use_name ? TRUE : FALSE;
      g_object_notify (G_OBJECT (store), "use-name");
    }
}

gboolean
gimp_container_tree_store_get_use_name (GimpContainerTreeStore *store)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store), FALSE);

  return GET_PRIVATE (store)->use_name;
}

static gboolean
gimp_container_tree_store_set_context_foreach (GtkTreeModel *model,
                                               GtkTreePath  *path,
                                               GtkTreeIter  *iter,
                                               gpointer      data)
{
  GimpContext      *context = data;
  GimpViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  gimp_view_renderer_set_context (renderer, context);

  g_object_unref (renderer);

  return FALSE;
}

void
gimp_container_tree_store_set_context (GimpContainerTreeStore *store,
                                       GimpContext            *context)
{
  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));

  gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                          gimp_container_tree_store_set_context_foreach,
                          context);
}

GtkTreeIter *
gimp_container_tree_store_insert_item (GimpContainerTreeStore *store,
                                       GimpViewable           *viewable,
                                       GtkTreeIter            *parent,
                                       gint                    index)
{
  GtkTreeIter iter;

  g_return_val_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store), NULL);

  if (index == -1)
    gtk_tree_store_append (GTK_TREE_STORE (store), &iter, parent);
  else
    gtk_tree_store_insert (GTK_TREE_STORE (store), &iter, parent, index);

  gimp_container_tree_store_set (store, &iter, viewable);

  return gtk_tree_iter_copy (&iter);
}

void
gimp_container_tree_store_remove_item (GimpContainerTreeStore *store,
                                       GimpViewable           *viewable,
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
          GimpContainerTreeStorePrivate *private = GET_PRIVATE (store);
          GList                         *list;

          for (list = private->renderer_cells; list; list = list->next)
            g_object_set (list->data, "renderer", NULL, NULL);
        }
    }
}

void
gimp_container_tree_store_reorder_item (GimpContainerTreeStore *store,
                                        GimpViewable           *viewable,
                                        gint                    new_index,
                                        GtkTreeIter            *iter)
{
  GimpContainerTreeStorePrivate *private;
  GimpViewable                  *parent;
  GimpContainer                 *container;

  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  if (! iter)
    return;

  parent = gimp_viewable_get_parent (viewable);

  if (parent)
    container = gimp_viewable_get_children (parent);
  else
    container = gimp_container_view_get_container (private->container_view);

  if (new_index == -1 ||
      new_index == gimp_container_get_n_children (container) - 1)
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
gimp_container_tree_store_rename_item (GimpContainerTreeStore *store,
                                       GimpViewable           *viewable,
                                       GtkTreeIter            *iter)
{
  gboolean new_name_shorter = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store), FALSE);

  if (iter)
    {
      GimpContainerTreeStorePrivate *private = GET_PRIVATE (store);
      gchar                         *name;
      gchar                         *old_name;

      if (private->use_name)
        name = (gchar *) gimp_object_get_name (viewable);
      else
        name = gimp_viewable_get_description (viewable, NULL);

      gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, &old_name,
                          -1);

      gtk_tree_store_set (GTK_TREE_STORE (store), iter,
                          GIMP_CONTAINER_TREE_STORE_COLUMN_NAME, name,
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
gimp_container_tree_store_clear_items (GimpContainerTreeStore *store)
{
  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));

  gtk_tree_store_clear (GTK_TREE_STORE (store));

  /*  If the store is empty after this remove, clear out renderers
   *  from all cells so they don't keep refing the viewables
   *  (see bug #149906).
   */
  if (! gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL))
    {
      GimpContainerTreeStorePrivate *private = GET_PRIVATE (store);
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
gimp_container_tree_store_set_view_size_foreach (GtkTreeModel *model,
                                                 GtkTreePath  *path,
                                                 GtkTreeIter  *iter,
                                                 gpointer      data)
{
  SetSizeForeachData *size_data = data;
  GimpViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  gimp_view_renderer_set_size (renderer,
                               size_data->view_size,
                               size_data->border_width);

  g_object_unref (renderer);

  return FALSE;
}

void
gimp_container_tree_store_set_view_size (GimpContainerTreeStore *store)
{
  GimpContainerTreeStorePrivate *private;
  SetSizeForeachData             size_data;

  g_return_if_fail (GIMP_IS_CONTAINER_TREE_STORE (store));

  private = GET_PRIVATE (store);

  size_data.view_size =
    gimp_container_view_get_view_size (private->container_view,
                                       &size_data.border_width);

  gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                          gimp_container_tree_store_set_view_size_foreach,
                          &size_data);
}


/*  private functions  */

void
gimp_container_tree_store_columns_init (GType *types,
                                        gint  *n_types)
{
  g_return_if_fail (types != NULL);
  g_return_if_fail (n_types != NULL);
  g_return_if_fail (*n_types == 0);

  gimp_assert (GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER ==
               gimp_container_tree_store_columns_add (types, n_types,
                                                      GIMP_TYPE_VIEW_RENDERER));

  gimp_assert (GIMP_CONTAINER_TREE_STORE_COLUMN_NAME ==
               gimp_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_STRING));

  gimp_assert (GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES ==
               gimp_container_tree_store_columns_add (types, n_types,
                                                      PANGO_TYPE_ATTR_LIST));

  gimp_assert (GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE ==
               gimp_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_BOOLEAN));

  gimp_assert (GIMP_CONTAINER_TREE_STORE_COLUMN_USER_DATA ==
               gimp_container_tree_store_columns_add (types, n_types,
                                                      G_TYPE_POINTER));
}

gint
gimp_container_tree_store_columns_add (GType *types,
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
gimp_container_tree_store_set (GimpContainerTreeStore *store,
                               GtkTreeIter            *iter,
                               GimpViewable           *viewable)
{
  GimpContainerTreeStorePrivate *private = GET_PRIVATE (store);
  GimpContext                   *context;
  GimpViewRenderer              *renderer;
  gchar                         *name;
  gint                           view_size;
  gint                           border_width;

  context = gimp_container_view_get_context (private->container_view);

  view_size = gimp_container_view_get_view_size (private->container_view,
                                                 &border_width);

  renderer = gimp_view_renderer_new (context,
                                     G_TYPE_FROM_INSTANCE (viewable),
                                     view_size, border_width,
                                     FALSE);
  gimp_view_renderer_set_viewable (renderer, viewable);
  gimp_view_renderer_remove_idle (renderer);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (gimp_container_tree_store_renderer_update),
                    store);

  if (private->use_name)
    name = (gchar *) gimp_object_get_name (viewable);
  else
    name = gimp_viewable_get_description (viewable, NULL);

  gtk_tree_store_set (GTK_TREE_STORE (store), iter,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_RENDERER,       renderer,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME,           name,
                      GIMP_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE, TRUE,
                      -1);

  if (! private->use_name)
    g_free (name);

  g_object_unref (renderer);
}

static void
gimp_container_tree_store_renderer_update (GimpViewRenderer       *renderer,
                                           GimpContainerTreeStore *store)
{
  GimpContainerTreeStorePrivate *private = GET_PRIVATE (store);
  GtkTreeIter                   *iter;

  iter = gimp_container_view_lookup (private->container_view,
                                     renderer->viewable);

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
      gtk_tree_model_row_changed (GTK_TREE_MODEL (store), path, iter);
      gtk_tree_path_free (path);
    }
}
