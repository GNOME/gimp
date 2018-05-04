/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumstore.c
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpenumstore.h"


/**
 * SECTION: gimpenumstore
 * @title: GimpEnumStore
 * @short_description: A #GimpIntStore subclass that keeps enum values.
 *
 * A #GimpIntStore subclass that keeps enum values.
 **/


enum
{
  PROP_0,
  PROP_ENUM_TYPE
};


struct _GimpEnumStorePrivate
{
  GEnumClass *enum_class;
};

#define GET_PRIVATE(obj) (((GimpEnumStore *) (obj))->priv)


static void   gimp_enum_store_finalize     (GObject      *object);
static void   gimp_enum_store_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   gimp_enum_store_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

static void   gimp_enum_store_add_value    (GtkListStore *store,
                                            GEnumValue   *value);


G_DEFINE_TYPE (GimpEnumStore, gimp_enum_store, GIMP_TYPE_INT_STORE)

#define parent_class gimp_enum_store_parent_class


static void
gimp_enum_store_class_init (GimpEnumStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_enum_store_finalize;
  object_class->set_property = gimp_enum_store_set_property;
  object_class->get_property = gimp_enum_store_get_property;

  /**
   * GimpEnumStore:enum-type:
   *
   * Sets the #GType of the enum to be used in the store.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ENUM_TYPE,
                                   g_param_spec_gtype ("enum-type",
                                                       "Enum Type",
                                                       "The type of the enum",
                                                       G_TYPE_ENUM,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpEnumStorePrivate));
}

static void
gimp_enum_store_init (GimpEnumStore *store)
{
  store->priv = G_TYPE_INSTANCE_GET_PRIVATE (store,
                                             GIMP_TYPE_ENUM_STORE,
                                             GimpEnumStorePrivate);
}

static void
gimp_enum_store_finalize (GObject *object)
{
  GimpEnumStorePrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->enum_class, g_type_class_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_enum_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpEnumStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      g_return_if_fail (priv->enum_class == NULL);
      priv->enum_class = g_type_class_ref (g_value_get_gtype (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_enum_store_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpEnumStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      g_value_set_gtype (value, (priv->enum_class ?
                                 G_TYPE_FROM_CLASS (priv->enum_class) :
                                 G_TYPE_NONE));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_enum_store_add_value (GtkListStore *store,
                           GEnumValue   *value)
{
  GimpEnumStorePrivate *priv = GET_PRIVATE (store);
  GtkTreeIter           iter = { 0, };
  const gchar          *desc;
  const gchar          *abbrev;
  gchar                *stripped;

  desc   = gimp_enum_value_get_desc   (priv->enum_class, value);
  abbrev = gimp_enum_value_get_abbrev (priv->enum_class, value);

  /* no mnemonics in combo boxes */
  stripped = gimp_strip_uline (desc);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      GIMP_INT_STORE_VALUE,  value->value,
                      GIMP_INT_STORE_LABEL,  stripped,
                      GIMP_INT_STORE_ABBREV, abbrev,
                      -1);

  g_free (stripped);
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
 *
 * Since: 2.4
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

/**
 * gimp_enum_store_new_with_range:
 * @enum_type: the #GType of an enum.
 * @minimum: the minimum value to include
 * @maximum: the maximum value to include
 *
 * Creates a new #GimpEnumStore like gimp_enum_store_new() but allows
 * to limit the enum values to a certain range. Values smaller than
 * @minimum or larger than @maximum are not added to the store.
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_range (GType  enum_type,
                                gint   minimum,
                                gint   maximum)
{
  GimpEnumStorePrivate *priv;
  GtkListStore         *store;
  GEnumValue           *value;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  priv = GET_PRIVATE (store);

  for (value = priv->enum_class->values;
       value->value_name;
       value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      gimp_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * gimp_enum_store_new_with_values
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @...:       a list of enum values (exactly @n_values)
 *
 * Creates a new #GimpEnumStore like gimp_enum_store_new() but allows
 * to explicitly list the enum values that should be added to the
 * store.
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_values (GType enum_type,
                                 gint  n_values,
                                 ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, n_values);

  store = gimp_enum_store_new_with_values_valist (enum_type, n_values, args);

  va_end (args);

  return store;
}

/**
 * gimp_enum_store_new_with_values_valist:
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @args:      a va_list of enum values (exactly @n_values)
 *
 * See gimp_enum_store_new_with_values().
 *
 * Return value: a new #GimpEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
gimp_enum_store_new_with_values_valist (GType     enum_type,
                                        gint      n_values,
                                        va_list   args)
{
  GimpEnumStorePrivate *priv;
  GtkListStore         *store;
  GEnumValue           *value;
  gint                  i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  priv = GET_PRIVATE (store);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (priv->enum_class,
                                va_arg (args, gint));

      if (value)
        gimp_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * gimp_enum_store_set_icon_prefix:
 * @store:       a #GimpEnumStore
 * @icon_prefix: a prefix to create icon names from enum values
 *
 * Creates an icon name for each enum value in the @store by appending
 * the value's nick to the given @icon_prefix, separated by a hyphen.
 *
 * See also: gimp_enum_combo_box_set_icon_prefix().
 *
 * Since: 2.10
 **/
void
gimp_enum_store_set_icon_prefix (GimpEnumStore *store,
                                 const gchar   *icon_prefix)
{
  GimpEnumStorePrivate *priv;
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;

  g_return_if_fail (GIMP_IS_ENUM_STORE (store));

  priv  = GET_PRIVATE (store);
  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gchar *icon_name = NULL;

      if (icon_prefix)
        {
          GEnumValue *enum_value;
          gint        value;

          gtk_tree_model_get (model, &iter,
                              GIMP_INT_STORE_VALUE, &value,
                              -1);

          enum_value = g_enum_get_value (priv->enum_class, value);

          if (enum_value)
            {
              icon_name = g_strconcat (icon_prefix, "-",
                                       enum_value->value_nick,
                                       NULL);
            }
        }

      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                          GIMP_INT_STORE_ICON_NAME, icon_name,
                          -1);

      if (icon_name)
        g_free (icon_name);
    }
}
