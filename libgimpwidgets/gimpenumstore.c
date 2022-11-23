/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumstore.c
 * Copyright (C) 2004-2007  Sven Neumann <sven@ligma.org>
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

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaenumstore.h"


/**
 * SECTION: ligmaenumstore
 * @title: LigmaEnumStore
 * @short_description: A #LigmaIntStore subclass that keeps enum values.
 *
 * A #LigmaIntStore subclass that keeps enum values.
 **/


enum
{
  PROP_0,
  PROP_ENUM_TYPE
};


struct _LigmaEnumStorePrivate
{
  GEnumClass *enum_class;
};

#define GET_PRIVATE(obj) (((LigmaEnumStore *) (obj))->priv)


static void   ligma_enum_store_finalize     (GObject      *object);
static void   ligma_enum_store_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   ligma_enum_store_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

static void   ligma_enum_store_add_value    (GtkListStore *store,
                                            GEnumValue   *value);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaEnumStore, ligma_enum_store, LIGMA_TYPE_INT_STORE)

#define parent_class ligma_enum_store_parent_class


static void
ligma_enum_store_class_init (LigmaEnumStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_enum_store_finalize;
  object_class->set_property = ligma_enum_store_set_property;
  object_class->get_property = ligma_enum_store_get_property;

  /**
   * LigmaEnumStore:enum-type:
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
                                                       LIGMA_PARAM_READWRITE));
}

static void
ligma_enum_store_init (LigmaEnumStore *store)
{
  store->priv = ligma_enum_store_get_instance_private (store);
}

static void
ligma_enum_store_finalize (GObject *object)
{
  LigmaEnumStorePrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->enum_class, g_type_class_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_enum_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaEnumStorePrivate *priv = GET_PRIVATE (object);

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
ligma_enum_store_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaEnumStorePrivate *priv = GET_PRIVATE (object);

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
ligma_enum_store_add_value (GtkListStore *store,
                           GEnumValue   *value)
{
  LigmaEnumStorePrivate *priv = GET_PRIVATE (store);
  GtkTreeIter           iter = { 0, };
  const gchar          *desc;
  const gchar          *abbrev;
  gchar                *stripped;

  desc   = ligma_enum_value_get_desc   (priv->enum_class, value);
  abbrev = ligma_enum_value_get_abbrev (priv->enum_class, value);

  /* no mnemonics in combo boxes */
  stripped = ligma_strip_uline (desc);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      LIGMA_INT_STORE_VALUE,  value->value,
                      LIGMA_INT_STORE_LABEL,  stripped,
                      LIGMA_INT_STORE_ABBREV, abbrev,
                      -1);

  g_free (stripped);
}


/**
 * ligma_enum_store_new:
 * @enum_type: the #GType of an enum.
 *
 * Creates a new #LigmaEnumStore, derived from #GtkListStore and fills
 * it with enum values. The enum needs to be registered to the type
 * system and should have translatable value names.
 *
 * Returns: a new #LigmaEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
ligma_enum_store_new (GType enum_type)
{
  GtkListStore *store;
  GEnumClass   *enum_class;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  store = ligma_enum_store_new_with_range (enum_type,
                                          enum_class->minimum,
                                          enum_class->maximum);

  g_type_class_unref (enum_class);

  return store;
}

/**
 * ligma_enum_store_new_with_range:
 * @enum_type: the #GType of an enum.
 * @minimum: the minimum value to include
 * @maximum: the maximum value to include
 *
 * Creates a new #LigmaEnumStore like ligma_enum_store_new() but allows
 * to limit the enum values to a certain range. Values smaller than
 * @minimum or larger than @maximum are not added to the store.
 *
 * Returns: a new #LigmaEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
ligma_enum_store_new_with_range (GType  enum_type,
                                gint   minimum,
                                gint   maximum)
{
  LigmaEnumStorePrivate *priv;
  GtkListStore         *store;
  GEnumValue           *value;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  store = g_object_new (LIGMA_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  priv = GET_PRIVATE (store);

  for (value = priv->enum_class->values;
       value->value_name;
       value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      ligma_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * ligma_enum_store_new_with_values: (skip)
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @...:       a list of enum values (exactly @n_values)
 *
 * Creates a new #LigmaEnumStore like ligma_enum_store_new() but allows
 * to explicitly list the enum values that should be added to the
 * store.
 *
 * Returns: a new #LigmaEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
ligma_enum_store_new_with_values (GType enum_type,
                                 gint  n_values,
                                 ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, n_values);

  store = ligma_enum_store_new_with_values_valist (enum_type, n_values, args);

  va_end (args);

  return store;
}

/**
 * ligma_enum_store_new_with_values_valist: (skip)
 * @enum_type: the #GType of an enum.
 * @n_values:  the number of enum values to include
 * @args:      a va_list of enum values (exactly @n_values)
 *
 * See ligma_enum_store_new_with_values().
 *
 * Returns: a new #LigmaEnumStore.
 *
 * Since: 2.4
 **/
GtkListStore *
ligma_enum_store_new_with_values_valist (GType     enum_type,
                                        gint      n_values,
                                        va_list   args)
{
  LigmaEnumStorePrivate *priv;
  GtkListStore         *store;
  GEnumValue           *value;
  gint                  i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);

  store = g_object_new (LIGMA_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  priv = GET_PRIVATE (store);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (priv->enum_class,
                                va_arg (args, gint));

      if (value)
        ligma_enum_store_add_value (store, value);
    }

  return store;
}

/**
 * ligma_enum_store_set_icon_prefix:
 * @store:       a #LigmaEnumStore
 * @icon_prefix: a prefix to create icon names from enum values
 *
 * Creates an icon name for each enum value in the @store by appending
 * the value's nick to the given @icon_prefix, separated by a hyphen.
 *
 * See also: ligma_enum_combo_box_set_icon_prefix().
 *
 * Since: 2.10
 **/
void
ligma_enum_store_set_icon_prefix (LigmaEnumStore *store,
                                 const gchar   *icon_prefix)
{
  LigmaEnumStorePrivate *priv;
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;

  g_return_if_fail (LIGMA_IS_ENUM_STORE (store));

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
                              LIGMA_INT_STORE_VALUE, &value,
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
                          LIGMA_INT_STORE_ICON_NAME, icon_name,
                          -1);

      if (icon_name)
        g_free (icon_name);
    }
}
