/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintstore.c
 * Copyright (C) 2004-2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpintstore.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpintstore
 * @title: GimpIntStore
 * @short_description: A model for integer based name-value pairs
 *                     (e.g. enums)
 *
 * A model for integer based name-value pairs (e.g. enums)
 **/


enum
{
  PROP_0,
  PROP_USER_DATA_TYPE
};


typedef struct _GimpIntStorePrivate
{
  GtkListStore parent_instance;

  GtkTreeIter *empty_iter;
  GType        user_data_type;
} GimpIntStorePrivate;

#define GET_PRIVATE(obj) ((GimpIntStorePrivate *) gimp_int_store_get_instance_private ((GimpIntStore *) (obj)))


static void  gimp_int_store_tree_model_init (GtkTreeModelIface *iface);

static void  gimp_int_store_constructed     (GObject           *object);
static void  gimp_int_store_finalize        (GObject           *object);
static void  gimp_int_store_set_property    (GObject           *object,
                                             guint              property_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);
static void  gimp_int_store_get_property    (GObject           *object,
                                             guint              property_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);

static void  gimp_int_store_row_inserted    (GtkTreeModel      *model,
                                             GtkTreePath       *path,
                                             GtkTreeIter       *iter);
static void  gimp_int_store_row_deleted     (GtkTreeModel      *model,
                                             GtkTreePath       *path);
static void  gimp_int_store_add_empty       (GimpIntStore      *store);


G_DEFINE_TYPE_WITH_CODE (GimpIntStore, gimp_int_store, GTK_TYPE_LIST_STORE,
                         G_ADD_PRIVATE (GimpIntStore)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                gimp_int_store_tree_model_init))

#define parent_class gimp_int_store_parent_class

static GtkTreeModelIface *parent_iface = NULL;


static void
gimp_int_store_class_init (GimpIntStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_int_store_constructed;
  object_class->finalize     = gimp_int_store_finalize;
  object_class->set_property = gimp_int_store_set_property;
  object_class->get_property = gimp_int_store_get_property;

  /**
   * GimpIntStore:user-data-type:
   *
   * Sets the #GType for the GIMP_INT_STORE_USER_DATA column.
   *
   * You need to set this property when constructing the store if you want
   * to use the GIMP_INT_STORE_USER_DATA column and want to have the store
   * handle ref-counting of your user data.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_USER_DATA_TYPE,
                                   g_param_spec_gtype ("user-data-type",
                                                       "User Data Type",
                                                       "The GType of the user_data column",
                                                       G_TYPE_NONE,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       GIMP_PARAM_READWRITE));
}

static void
gimp_int_store_tree_model_init (GtkTreeModelIface *iface)
{
  parent_iface = g_type_interface_peek_parent (iface);

  iface->row_inserted = gimp_int_store_row_inserted;
  iface->row_deleted  = gimp_int_store_row_deleted;
}

static void
gimp_int_store_init (GimpIntStore *store)
{
}

static void
gimp_int_store_constructed (GObject *object)
{
  GimpIntStore        *store = GIMP_INT_STORE (object);
  GimpIntStorePrivate *priv  = GET_PRIVATE (store);
  GType                types[GIMP_INT_STORE_NUM_COLUMNS];

  G_OBJECT_CLASS (parent_class)->constructed (object);

  types[GIMP_INT_STORE_VALUE]     = G_TYPE_INT;
  types[GIMP_INT_STORE_LABEL]     = G_TYPE_STRING;
  types[GIMP_INT_STORE_ABBREV]    = G_TYPE_STRING;
  types[GIMP_INT_STORE_ICON_NAME] = G_TYPE_STRING;
  types[GIMP_INT_STORE_PIXBUF]    = GDK_TYPE_PIXBUF;
  types[GIMP_INT_STORE_USER_DATA] = (priv->user_data_type != G_TYPE_NONE ?
                                     priv->user_data_type : G_TYPE_POINTER);

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   GIMP_INT_STORE_NUM_COLUMNS, types);

  gimp_int_store_add_empty (store);
}

static void
gimp_int_store_finalize (GObject *object)
{
  GimpIntStorePrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->empty_iter, gtk_tree_iter_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_int_store_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpIntStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_USER_DATA_TYPE:
      priv->user_data_type = g_value_get_gtype (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_int_store_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpIntStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_USER_DATA_TYPE:
      g_value_set_gtype (value, priv->user_data_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_int_store_row_inserted (GtkTreeModel *model,
                             GtkTreePath  *path,
                             GtkTreeIter  *iter)
{
  GimpIntStore        *store = GIMP_INT_STORE (model);
  GimpIntStorePrivate *priv  = GET_PRIVATE (store);

  if (parent_iface->row_inserted)
    parent_iface->row_inserted (model, path, iter);

  if (priv->empty_iter &&
      memcmp (iter, priv->empty_iter, sizeof (GtkTreeIter)))
    {
      gtk_list_store_remove (GTK_LIST_STORE (store), priv->empty_iter);
      gtk_tree_iter_free (priv->empty_iter);
      priv->empty_iter = NULL;
    }
}

static void
gimp_int_store_row_deleted (GtkTreeModel *model,
                            GtkTreePath  *path)
{
  if (parent_iface->row_deleted)
    parent_iface->row_deleted (model, path);
}

static void
gimp_int_store_add_empty (GimpIntStore *store)
{
  GimpIntStorePrivate *priv = GET_PRIVATE (store);
  GtkTreeIter          iter = { 0, };

  g_return_if_fail (priv->empty_iter == NULL);

  gtk_list_store_prepend (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_INT_STORE_VALUE, -1,
                      /* This string appears in an empty menu as in
                       * "nothing selected and nothing to select"
                       */
                      GIMP_INT_STORE_LABEL, (_("(Empty)")),
                      -1);

  priv->empty_iter = gtk_tree_iter_copy (&iter);
}

/**
 * gimp_int_store_new: (skip)
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @...:         a %NULL terminated list of more label, value pairs
 *
 * Creates a #GtkListStore with a number of useful columns.
 * #GimpIntStore is especially useful if the items you want to store
 * are identified using an integer value.
 *
 * If you need to construct an empty #GimpIntStore, it's best to use
 * g_object_new (GIMP_TYPE_INT_STORE, NULL).
 *
 * Returns: a new #GimpIntStore.
 *
 * Since: 2.2
 **/
GtkListStore *
gimp_int_store_new (const gchar *first_label,
                    gint         first_value,
                    ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, first_value);

  store = gimp_int_store_new_valist (first_label, first_value, args);

  va_end (args);

  return store;
}

/**
 * gimp_int_store_new_valist: (skip)
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @values:      a va_list with more values
 *
 * A variant of gimp_int_store_new() that takes a va_list of
 * label/value pairs.
 *
 * Returns: a new #GimpIntStore.
 *
 * Since: 3.0
 **/
GtkListStore *
gimp_int_store_new_valist (const gchar *first_label,
                           gint         first_value,
                           va_list      values)
{
  GtkListStore *store;
  const gchar  *label;
  gint          value;

  store = g_object_new (GIMP_TYPE_INT_STORE, NULL);

  for (label = first_label, value = first_value;
       label;
       label = va_arg (values, const gchar *), value = va_arg (values, gint))
    {
      GtkTreeIter iter = { 0, };

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          GIMP_INT_STORE_VALUE, value,
                          GIMP_INT_STORE_LABEL, label,
                          -1);
    }

  return store;
}

/**
 * gimp_int_store_new_array: (rename-to gimp_int_store_new)
 * @n_values: the number of values
 * @labels: (array length=n_values): an array of labels
 *
 * A variant of gimp_int_store_new() that takes an array of labels.
 * The array indices are used as values.
 *
 * Returns: a new #GtkListStore.
 *
 * Since: 3.0
 **/
GtkListStore *
gimp_int_store_new_array (gint         n_values,
                          const gchar *labels[])
{
  GtkListStore *store;
  gint          i;

  g_return_val_if_fail (n_values >= 0, NULL);
  g_return_val_if_fail (labels != NULL || n_values == 0, NULL);

  store = g_object_new (GIMP_TYPE_INT_STORE, NULL);

  for (i = 0; i < n_values; i++)
    {
      GtkTreeIter  iter;

      if (labels[i])
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_VALUE, i,
                              GIMP_INT_STORE_LABEL, gettext (labels[i]),
                              -1);
        }
    }

  return store;
}

/**
 * gimp_int_store_lookup_by_value:
 * @model: a #GimpIntStore
 * @value: an integer value to lookup in the @model
 * @iter: (out):  return location for the iter of the given @value
 *
 * Iterate over the @model looking for @value.
 *
 * Returns: %TRUE if the value has been located and @iter is
 *               valid, %FALSE otherwise.
 *
 * Since: 2.2
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

/**
 * gimp_int_store_lookup_by_user_data:
 * @model: a #GimpIntStore
 * @user_data: a gpointer "user-data" to lookup in the @model
 * @iter: (out): return location for the iter of the given @user_data
 *
 * Iterate over the @model looking for @user_data.
 *
 * Returns: %TRUE if the user-data has been located and @iter is
 *               valid, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_int_store_lookup_by_user_data (GtkTreeModel *model,
                                    gpointer      user_data,
                                    GtkTreeIter  *iter)
{
  gboolean iter_valid = FALSE;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gpointer this;

      gtk_tree_model_get (model, iter,
                          GIMP_INT_STORE_USER_DATA, &this,
                          -1);
      if (this == user_data)
        break;
    }

  return (gboolean) iter_valid;
}
