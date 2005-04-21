/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpintstore.h"

#include "libgimp/libgimp-intl.h"


static void  gimp_int_store_class_init      (GimpIntStoreClass *klass);
static void  gimp_int_store_tree_model_init (GtkTreeModelIface *iface);
static void  gimp_int_store_init            (GimpIntStore      *store);
static void  gimp_int_store_finalize        (GObject           *object);
static void  gimp_int_store_row_inserted    (GtkTreeModel      *model,
                                             GtkTreePath       *path,
                                             GtkTreeIter       *iter);
static void  gimp_int_store_add_empty       (GimpIntStore      *store);


static GtkListStoreClass *parent_class = NULL;
static GtkTreeModelIface *parent_iface = NULL;


GType
gimp_int_store_get_type (void)
{
  static GType store_type = 0;

  if (! store_type)
    {
      static const GTypeInfo store_info =
      {
        sizeof (GimpIntStoreClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_int_store_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpIntStore),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_int_store_init
      };
      static const GInterfaceInfo iface_info =
      {
        (GInterfaceInitFunc) gimp_int_store_tree_model_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      store_type = g_type_register_static (GTK_TYPE_LIST_STORE,
                                           "GimpIntStore",
                                           &store_info, 0);

      g_type_add_interface_static (store_type, GTK_TYPE_TREE_MODEL,
                                   &iface_info);
    }

  return store_type;
}

static void
gimp_int_store_class_init (GimpIntStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_int_store_finalize;
}

static void
gimp_int_store_tree_model_init (GtkTreeModelIface *iface)
{
  parent_iface = g_type_interface_peek_parent (iface);

  iface->row_inserted = gimp_int_store_row_inserted;
}

static void
gimp_int_store_init (GimpIntStore *store)
{
  GType types[GIMP_INT_STORE_NUM_COLUMNS];

  types[GIMP_INT_STORE_VALUE]     = G_TYPE_INT;
  types[GIMP_INT_STORE_LABEL]     = G_TYPE_STRING;
  types[GIMP_INT_STORE_STOCK_ID]  = G_TYPE_STRING;
  types[GIMP_INT_STORE_PIXBUF]    = GDK_TYPE_PIXBUF;
  types[GIMP_INT_STORE_USER_DATA] = G_TYPE_POINTER;

  store->empty_iter = NULL;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   GIMP_INT_STORE_NUM_COLUMNS, types);

  gimp_int_store_add_empty (store);
}

static void
gimp_int_store_finalize (GObject *object)
{
  GimpIntStore *store = GIMP_INT_STORE (object);

  if (store->empty_iter)
    {
      gtk_tree_iter_free (store->empty_iter);
      store->empty_iter = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_int_store_row_inserted (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter)
{
  GimpIntStore *store = GIMP_INT_STORE (model);

  if (parent_iface->row_inserted)
    parent_iface->row_inserted (model, path, iter);

  if (store->empty_iter &&
      memcmp (iter, store->empty_iter, sizeof (GtkTreeIter)))
    {
      gtk_list_store_remove (GTK_LIST_STORE (store), store->empty_iter);
      gtk_tree_iter_free (store->empty_iter);
      store->empty_iter = NULL;
    }
}

static void
gimp_int_store_add_empty (GimpIntStore *store)
{
  g_return_if_fail (store->empty_iter == NULL);

  store->empty_iter = g_new0 (GtkTreeIter, 1);

  gtk_list_store_prepend (GTK_LIST_STORE (store), store->empty_iter);
  gtk_list_store_set (GTK_LIST_STORE (store), store->empty_iter,
                      GIMP_INT_STORE_VALUE, -1,
                      GIMP_INT_STORE_LABEL, (_("(Empty)")),
                      -1);
}

/**
 * gimp_int_store_new:
 *
 * Creates a #GtkListStore with a number of useful columns.
 * #GimpIntStore is especially useful if the items you want to store
 * are identified using an integer value.
 *
 * Return value: a new #GimpIntStore.
 *
 * Since: GIMP 2.2
 **/
GtkListStore *
gimp_int_store_new (void)
{
  return g_object_new (GIMP_TYPE_INT_STORE, NULL);
}

/**
 * gimp_int_store_lookup_by_value:
 * @model: a #GimpIntStore
 * @value: an integer value to lookup in the @model
 * @iter:  return location for the iter of the given @value
 *
 * Iterate over the @model looking for @value.
 *
 * Return value: %TRUE if the value has been located and @iter is
 *               valid, %FALSE otherwise.
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_int_store_lookup_by_value (GtkTreeModel *model,
                                gint          value,
                                GtkTreeIter  *iter)
{
  gboolean  iter_valid;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint  this;

      gtk_tree_model_get (model, iter,
                          GIMP_INT_STORE_VALUE, &this,
                          -1);
      if (this == value)
        break;
    }

  return iter_valid;
}
