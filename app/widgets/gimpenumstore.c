/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenumstore.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpenumstore.h"

#include "gimp-intl.h"


static void   gimp_enum_store_class_init   (GimpEnumStoreClass *klass);

static void   gimp_enum_store_init         (GimpEnumStore *enum_store);
static void   gimp_enum_store_finalize     (GObject       *object);

static void   gimp_enum_store_add_value    (GtkListStore  *store,
                                            GEnumValue    *value);


static GtkListStoreClass *parent_class = NULL;


GType
gimp_enum_store_get_type (void)
{
  static GType enum_store_type = 0;

  if (!enum_store_type)
    {
      static const GTypeInfo enum_store_info =
      {
        sizeof (GimpEnumStoreClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_enum_store_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpEnumStore),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_enum_store_init,
      };

      enum_store_type = g_type_register_static (GTK_TYPE_LIST_STORE,
                                                "GimpEnumStore",
                                                &enum_store_info, 0);
    }

  return enum_store_type;
}

static void
gimp_enum_store_class_init (GimpEnumStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_enum_store_finalize;
}

static void
gimp_enum_store_init (GimpEnumStore *enum_store)
{
  GType types[GIMP_ENUM_STORE_NUM_COLUMNS] =
    {
      G_TYPE_INT,       /*  GIMP_ENUM_STORE_VALUE      */
      G_TYPE_STRING,    /*  GIMP_ENUM_STORE_LABEL      */
      GDK_TYPE_PIXBUF,  /*  GIMP_ENUM_STORE_ICON       */
      G_TYPE_POINTER    /*  GIMP_ENUM_STORE_USER_DATA  */
    };

  enum_store->enum_class = NULL;

  gtk_list_store_set_column_types (GTK_LIST_STORE (enum_store),
                                   GIMP_ENUM_STORE_NUM_COLUMNS, types);
}

static void
gimp_enum_store_finalize (GObject *object)
{
  GimpEnumStore *enum_store = GIMP_ENUM_STORE (object);

  if (enum_store->enum_class)
    g_type_class_unref (enum_store->enum_class);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_enum_store_new:
 * @enum_type: the #GType of an enum.
 *
 * Creates a new #GimpEnumStore, derived from #GtkListStore and fills
 * it with enum values. The enum needs to be registered to the type
 * system and should have translatable value names.
 *
 * Return value: a new #GimpEnumStore.
 **/
GtkListStore *
gimp_enum_store_new (GType enum_type)
{
  GtkListStore *store;
  GEnumClass   *enum_class;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  store = gimp_enum_store_new_with_range (enum_type,
                                          enum_class->minimum,
                                          enum_class->maximum);

  g_type_class_unref (enum_class);

  return store;
}

GtkListStore *
gimp_enum_store_new_with_range (GType  enum_type,
                                gint   minimum,
                                gint   maximum)
{
  GtkListStore *store;
  GEnumValue   *value;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE, NULL);

  GIMP_ENUM_STORE (store)->enum_class = g_type_class_ref (enum_type);

  for (value = GIMP_ENUM_STORE (store)->enum_class->values;
       value->value_name;
       value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      gimp_enum_store_add_value (store, value);
    }

  return store;
}

GtkListStore *
gimp_enum_store_new_with_values (GType enum_type,
                                 gint  n_values,
                                 ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, n_values);

  store = gimp_enum_store_new_with_values_valist (enum_type,
                                                  n_values,
                                                  args);

  va_end (args);

  return store;
}

GtkListStore *
gimp_enum_store_new_with_values_valist (GType     enum_type,
                                        gint      n_values,
                                        va_list   args)
{
  GtkListStore *store;
  GEnumValue   *value;
  gint          i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE, NULL);

  GIMP_ENUM_STORE (store)->enum_class = g_type_class_ref (enum_type);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (GIMP_ENUM_STORE (store)->enum_class,
                                va_arg (args, gint));

      if (value)
        gimp_enum_store_add_value (store, value);
    }

  return store;
}

gboolean
gimp_enum_store_lookup_by_value (GtkTreeModel *model,
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
                          GIMP_ENUM_STORE_VALUE, &this,
                          -1);
      if (this == value)
        break;
    }

  return iter_valid;
}

void
gimp_enum_store_set_icons (GimpEnumStore *store,
                           GtkWidget     *widget,
                           const gchar   *stock_prefix,
                           GtkIconSize    size)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (GIMP_IS_ENUM_STORE (store));
  g_return_if_fail (stock_prefix == NULL || GTK_IS_WIDGET (widget));
  g_return_if_fail (size > GTK_ICON_SIZE_INVALID || size == -1);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GdkPixbuf *pixbuf = NULL;

      if (stock_prefix)
        {
          GEnumValue *enum_value;
          gchar      *stock_id;
          gint        value;

          gtk_tree_model_get (model, &iter,
                              GIMP_ENUM_STORE_VALUE, &value,
                              -1);

          enum_value = g_enum_get_value (store->enum_class, value);

          stock_id = g_strconcat (stock_prefix, "-",
                                  enum_value->value_nick,
                                  NULL);

          pixbuf = gtk_widget_render_icon (widget, stock_id, size, NULL);

          g_free (stock_id);
        }

      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                          GIMP_ENUM_STORE_ICON, pixbuf,
                          -1);

      if (pixbuf)
        g_object_unref (pixbuf);
    }
}


static void
gimp_enum_store_add_value (GtkListStore *store,
                           GEnumValue   *value)
{
  GtkTreeIter  iter;

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      GIMP_ENUM_STORE_VALUE,     value->value,
                      GIMP_ENUM_STORE_LABEL,     gettext (value->value_name),
                      GIMP_ENUM_STORE_ICON,      NULL,
                      GIMP_ENUM_STORE_USER_DATA, NULL,
                      -1);
}
