/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpintstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpintstore.h"


static void  gimp_int_store_init (GimpIntStore *store);


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
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpIntStore),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_int_store_init
      };

      store_type = g_type_register_static (GTK_TYPE_LIST_STORE,
                                           "GimpIntStore",
                                           &store_info, 0);
    }

  return store_type;
}

static void
gimp_int_store_init (GimpIntStore *store)
{
  GType types[GIMP_INT_STORE_NUM_COLUMNS] =
    {
      G_TYPE_INT,       /*  GIMP_INT_STORE_VALUE      */
      G_TYPE_STRING,    /*  GIMP_INT_STORE_LABEL      */
      G_TYPE_STRING,    /*  GIMP_INT_STORE_STOCK_ID   */
      GDK_TYPE_PIXBUF,  /*  GIMP_INT_STORE_PIXBUF     */
      G_TYPE_POINTER    /*  GIMP_INT_STORE_USER_DATA  */
    };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   GIMP_INT_STORE_NUM_COLUMNS, types);
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
