/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpunitstore.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpunitstore.h"


enum
{
  PROP_0,
  PROP_NUM_VALUES
};


static void         gimp_unit_store_tree_model_init (GtkTreeModelIface *iface);

static void         gimp_unit_store_finalize        (GObject      *object);
static void         gimp_unit_store_set_property    (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void         gimp_unit_store_get_property    (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static GtkTreeModelFlags gimp_unit_store_get_flags  (GtkTreeModel *tree_model);
static gint         gimp_unit_store_get_n_columns   (GtkTreeModel *tree_model);
static GType        gimp_unit_store_get_column_type (GtkTreeModel *tree_model,
                                                     gint          index);
static gboolean     gimp_unit_store_get_iter        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreePath  *path);
static GtkTreePath *gimp_unit_store_get_path        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static void    gimp_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     gint          column,
                                                     GValue       *value);
static gboolean     gimp_unit_store_iter_next       (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     gimp_unit_store_iter_children   (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent);
static gboolean     gimp_unit_store_iter_has_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gint         gimp_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     gimp_unit_store_iter_nth_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent,
                                                     gint          n);
static gboolean     gimp_unit_store_iter_parent     (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *child);


G_DEFINE_TYPE_WITH_CODE (GimpUnitStore, gimp_unit_store, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                gimp_unit_store_tree_model_init))

#define parent_class gimp_unit_store_parent_class


static GType column_types[GIMP_UNIT_STORE_UNIT_COLUMNS] =
{
  G_TYPE_INVALID,
  G_TYPE_DOUBLE,
  G_TYPE_INT,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING
};


static void
gimp_unit_store_class_init (GimpUnitStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  column_types[GIMP_UNIT_STORE_UNIT] = GIMP_TYPE_UNIT;

  object_class->finalize     = gimp_unit_store_finalize;
  object_class->set_property = gimp_unit_store_set_property;
  object_class->get_property = gimp_unit_store_get_property;

  g_object_class_install_property (object_class, PROP_NUM_VALUES,
                                   g_param_spec_int ("num-values",
                                                     NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_unit_store_init (GimpUnitStore *store)
{
}

static void
gimp_unit_store_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = gimp_unit_store_get_flags;
  iface->get_n_columns   = gimp_unit_store_get_n_columns;
  iface->get_column_type = gimp_unit_store_get_column_type;
  iface->get_iter        = gimp_unit_store_get_iter;
  iface->get_path        = gimp_unit_store_get_path;
  iface->get_value       = gimp_unit_store_tree_model_get_value;
  iface->iter_next       = gimp_unit_store_iter_next;
  iface->iter_children   = gimp_unit_store_iter_children;
  iface->iter_has_child  = gimp_unit_store_iter_has_child;
  iface->iter_n_children = gimp_unit_store_iter_n_children;
  iface->iter_nth_child  = gimp_unit_store_iter_nth_child;
  iface->iter_parent     = gimp_unit_store_iter_parent;
}

static void
gimp_unit_store_finalize (GObject *object)
{
  GimpUnitStore *store = GIMP_UNIT_STORE (object);

  if (store->num_values > 0)
    {
      g_free (store->values);
      g_free (store->resolutions);
      store->num_values = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_unit_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpUnitStore *store = GIMP_UNIT_STORE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_return_if_fail (store->num_values == 0);
      store->num_values = g_value_get_int (value);
      if (store->num_values)
        {
          store->values      = g_new0 (gdouble, store->num_values);
          store->resolutions = g_new0 (gdouble, store->num_values);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_unit_store_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpUnitStore *store = GIMP_UNIT_STORE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_value_set_int (value, store->num_values);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkTreeModelFlags
gimp_unit_store_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

static gint
gimp_unit_store_get_n_columns (GtkTreeModel *tree_model)
{
  GimpUnitStore *store = GIMP_UNIT_STORE (tree_model);

  return GIMP_UNIT_STORE_UNIT_COLUMNS + store->num_values;
}

static GType
gimp_unit_store_get_column_type (GtkTreeModel *tree_model,
                                 gint          index)
{
  g_return_val_if_fail (index >= 0, G_TYPE_INVALID);

  if (index < GIMP_UNIT_STORE_UNIT_COLUMNS)
    return column_types[index];

  return G_TYPE_DOUBLE;
}

static gboolean
gimp_unit_store_get_iter (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter,
                          GtkTreePath  *path)
{
  gint  unit;

  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  unit = gtk_tree_path_get_indices (path)[0];

  if (unit >= 0 && unit < gimp_unit_get_number_of_units ())
    {
      iter->user_data = GINT_TO_POINTER (unit);
      return TRUE;
    }

  return FALSE;
}

static GtkTreePath *
gimp_unit_store_get_path (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter)
{
  GtkTreePath *path = gtk_tree_path_new ();

  gtk_tree_path_append_index (path, GPOINTER_TO_INT (iter->user_data));

  return path;
}

static void
gimp_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                      GtkTreeIter  *iter,
                                      gint          column,
                                      GValue       *value)
{
  GimpUnitStore *store = GIMP_UNIT_STORE (tree_model);
  GimpUnit       unit;

  g_return_if_fail (column >= 0 &&
                    column < GIMP_UNIT_STORE_UNIT_COLUMNS + store->num_values);

  g_value_init (value,
                column < GIMP_UNIT_STORE_UNIT_COLUMNS ?
                column_types[column] :
                G_TYPE_DOUBLE);

  unit = GPOINTER_TO_INT (iter->user_data);

  if (unit >= 0 && unit < gimp_unit_get_number_of_units ())
    {
      switch (column)
        {
        case GIMP_UNIT_STORE_UNIT:
          g_value_set_int (value, unit);
          break;
        case GIMP_UNIT_STORE_UNIT_FACTOR:
          g_value_set_double (value, gimp_unit_get_factor (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_DIGITS:
          g_value_set_int (value, gimp_unit_get_digits (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_IDENTIFIER:
          g_value_set_static_string (value, gimp_unit_get_identifier (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_SYMBOL:
          g_value_set_static_string (value, gimp_unit_get_symbol (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_ABBREVIATION:
          g_value_set_static_string (value, gimp_unit_get_abbreviation (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_SINGULAR:
          g_value_set_static_string (value, gimp_unit_get_singular (unit));
          break;
        case GIMP_UNIT_STORE_UNIT_PLURAL:
          g_value_set_static_string (value, gimp_unit_get_plural (unit));
          break;

        default:
          column -= GIMP_UNIT_STORE_UNIT_COLUMNS;
          if (unit == GIMP_UNIT_PIXEL)
            {
              g_value_set_double (value, store->values[column]);
            }
          else if (store->resolutions[column])
            {
              g_value_set_double (value,
                                  store->values[column] *
                                  gimp_unit_get_factor (unit) /
                                  store->resolutions[column]);
            }
          break;
        }
    }
}

static gboolean
gimp_unit_store_iter_next (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  gint  unit  = GPOINTER_TO_INT (iter->user_data);

  unit++;
  if (unit > 0 && unit < gimp_unit_get_number_of_units ())
    {
      iter->user_data = GINT_TO_POINTER (unit);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_unit_store_iter_children (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter,
                               GtkTreeIter  *parent)
{
  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  iter->user_data = GINT_TO_POINTER (0);

  return TRUE;
}

static gboolean
gimp_unit_store_iter_has_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter)
{
  return FALSE;
}

static gint
gimp_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
  if (iter)
    return 0;

  return gimp_unit_get_number_of_units ();
}

static gboolean
gimp_unit_store_iter_nth_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent,
                                gint          n)
{
  GimpUnitStore *store;

  if (parent)
    return FALSE;

  store = GIMP_UNIT_STORE (tree_model);

  if (n >= 0 && n < gimp_unit_get_number_of_units ())
    {
      iter->user_data = GINT_TO_POINTER (n);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_unit_store_iter_parent (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter,
                             GtkTreeIter  *child)
{
  return FALSE;
}


GimpUnitStore *
gimp_unit_store_new (gint  num_values)
{
  return g_object_new (GIMP_TYPE_UNIT_STORE,
                       "num-values", num_values,
                       NULL);
}

void
gimp_unit_store_set_pixel_value (GimpUnitStore *store,
                                 gint           index,
                                 gdouble        value)
{
  g_return_if_fail (GIMP_IS_UNIT_STORE (store));
  g_return_if_fail (index > 0 && index < store->num_values);

  store->values[index] = value;
}

void
gimp_unit_store_set_pixel_values (GimpUnitStore *store,
                                  gdouble        first_value,
                                  ...)
{
  va_list  args;
  gint     i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  va_start (args, first_value);

  for (i = 0; i < store->num_values; )
    {
      store->values[i] = first_value;

      if (++i < store->num_values)
        first_value = va_arg (args, gdouble);
    }

  va_end (args);
}

void
gimp_unit_store_set_resolution (GimpUnitStore *store,
                                gint           index,
                                gdouble        resolution)
{
  g_return_if_fail (GIMP_IS_UNIT_STORE (store));
  g_return_if_fail (index > 0 && index < store->num_values);

  store->resolutions[index] = resolution;
}

void
gimp_unit_store_set_resolutions  (GimpUnitStore *store,
                                  gdouble        first_resolution,
                                  ...)
{
  va_list  args;
  gint     i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  va_start (args, first_resolution);

  for (i = 0; i < store->num_values; )
    {
      store->resolutions[i] = first_resolution;

      if (++i < store->num_values)
        first_resolution = va_arg (args, gdouble);
    }

  va_end (args);
}

gdouble
gimp_unit_store_get_value (GimpUnitStore *store,
                           GimpUnit       unit,
                           gint           index)
{
  GtkTreeIter  iter;
  GValue       value = { 0, };

  g_return_val_if_fail (GIMP_IS_UNIT_STORE (store), 0.0);
  g_return_val_if_fail (index >= 0 && index < store->num_values, 0.0);

  iter.user_data = GINT_TO_POINTER (unit);

  gimp_unit_store_tree_model_get_value (GTK_TREE_MODEL (store),
                                        &iter,
                                        GIMP_UNIT_STORE_FIRST_VALUE + index,
                                        &value);

  return g_value_get_double (&value);
}

void
gimp_unit_store_get_values (GimpUnitStore *store,
                            GimpUnit       unit,
                            gdouble       *first_value,
                            ...)
{
  va_list  args;
  gint     i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  va_start (args, first_value);

  for (i = 0; i < store->num_values; )
    {
      if (first_value)
        *first_value = gimp_unit_store_get_value (store, unit, i);

      if (++i < store->num_values)
        first_value = va_arg (args, gdouble *);
    }

  va_end (args);
}
