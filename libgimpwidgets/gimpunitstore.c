/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include "gimpunitstore.h"


enum
{
  PROP_0,
  PROP_NUM_VALUES,
  PROP_HAS_PIXELS,
  PROP_HAS_PERCENT,
  PROP_SHORT_FORMAT,
  PROP_LONG_FORMAT
};

struct _GimpUnitStorePrivate
{
  gint      num_values;
  gboolean  has_pixels;
  gboolean  has_percent;

  gchar    *short_format;
  gchar    *long_format;

  gdouble  *values;
  gdouble  *resolutions;

  GimpUnit  synced_unit;
};

#define GET_PRIVATE(obj) (((GimpUnitStore *) (obj))->priv)


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
                                                     "Num Values",
                                                     "The number of values this store provides",
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HAS_PIXELS,
                                   g_param_spec_boolean ("has-pixels",
                                                         "Has Pixels",
                                                         "Whether the store has GIMP_UNIT_PIXELS",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_PERCENT,
                                   g_param_spec_boolean ("has-percent",
                                                         "Has Percent",
                                                         "Whether the store has GIMP_UNIT_PERCENT",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SHORT_FORMAT,
                                   g_param_spec_string ("short-format",
                                                        "Short Format",
                                                        "Format string for a short label",
                                                        "%a",
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_LONG_FORMAT,
                                   g_param_spec_string ("long-format",
                                                        "Long Format",
                                                        "Format string for a long label",
                                                        "%p",
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GimpUnitStorePrivate));
}

static void
gimp_unit_store_init (GimpUnitStore *store)
{
  GimpUnitStorePrivate *private;

  store->priv = G_TYPE_INSTANCE_GET_PRIVATE (store,
                                             GIMP_TYPE_UNIT_STORE,
                                             GimpUnitStorePrivate);

  private = store->priv;

  private->has_pixels   = TRUE;
  private->has_percent  = FALSE;
  private->short_format = g_strdup ("%a");
  private->long_format  = g_strdup ("%p");
  private->synced_unit  = gimp_unit_get_number_of_units () - 1;
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
  GimpUnitStorePrivate *private = GET_PRIVATE (object);

  if (private->short_format)
    {
      g_free (private->short_format);
      private->short_format = NULL;
    }

  if (private->long_format)
    {
      g_free (private->long_format);
      private->long_format = NULL;
    }

  if (private->num_values > 0)
    {
      g_free (private->values);
      g_free (private->resolutions);
      private->num_values = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_unit_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_return_if_fail (private->num_values == 0);
      private->num_values = g_value_get_int (value);
      if (private->num_values)
        {
          private->values      = g_new0 (gdouble, private->num_values);
          private->resolutions = g_new0 (gdouble, private->num_values);
        }
      break;
    case PROP_HAS_PIXELS:
      gimp_unit_store_set_has_pixels (GIMP_UNIT_STORE (object),
                                      g_value_get_boolean (value));
      break;
    case PROP_HAS_PERCENT:
      gimp_unit_store_set_has_percent (GIMP_UNIT_STORE (object),
                                       g_value_get_boolean (value));
      break;
    case PROP_SHORT_FORMAT:
      g_free (private->short_format);
      private->short_format = g_value_dup_string (value);
      if (! private->short_format)
        private->short_format = g_strdup ("%a");
      break;
    case PROP_LONG_FORMAT:
      g_free (private->long_format);
      private->long_format = g_value_dup_string (value);
      if (! private->long_format)
        private->long_format = g_strdup ("%a");
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
  GimpUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_value_set_int (value, private->num_values);
      break;
    case PROP_HAS_PIXELS:
      g_value_set_boolean (value, private->has_pixels);
      break;
    case PROP_HAS_PERCENT:
      g_value_set_boolean (value, private->has_percent);
      break;
    case PROP_SHORT_FORMAT:
      g_value_set_string (value, private->short_format);
      break;
    case PROP_LONG_FORMAT:
      g_value_set_string (value, private->long_format);
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
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);

  return GIMP_UNIT_STORE_UNIT_COLUMNS + private->num_values;
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
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  index;
  GimpUnit              unit;

  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  index = gtk_tree_path_get_indices (path)[0];

  unit = index;

  if (! private->has_pixels)
    unit++;

  if (private->has_percent)
    {
      unit--;

      if (private->has_pixels)
        {
          if (index == 0)
            unit = GIMP_UNIT_PIXEL;
          else if (index == 1)
            unit = GIMP_UNIT_PERCENT;
        }
      else
        {
          if (index == 0)
            unit = GIMP_UNIT_PERCENT;
        }
    }

  if ((unit >= 0 && unit < gimp_unit_get_number_of_units ()) ||
      ((unit == GIMP_UNIT_PERCENT && private->has_percent)))
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
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GtkTreePath          *path    = gtk_tree_path_new ();
  GimpUnit              unit    = GPOINTER_TO_INT (iter->user_data);
  gint                  index;

  index = unit;

  if (! private->has_pixels)
    index--;

  if (private->has_percent)
    {
      index++;

      if (private->has_pixels)
        {
          if (unit == GIMP_UNIT_PIXEL)
            index = 0;
          else if (unit == GIMP_UNIT_PERCENT)
            index = 1;
        }
      else
        {
          if (unit == GIMP_UNIT_PERCENT)
            index = 0;
        }
    }

  gtk_tree_path_append_index (path, index);

  return path;
}

static void
gimp_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                      GtkTreeIter  *iter,
                                      gint          column,
                                      GValue       *value)
{
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GimpUnit              unit;

  g_return_if_fail (column >= 0 &&
                    column < GIMP_UNIT_STORE_UNIT_COLUMNS + private->num_values);

  g_value_init (value,
                column < GIMP_UNIT_STORE_UNIT_COLUMNS ?
                column_types[column] :
                G_TYPE_DOUBLE);

  unit = GPOINTER_TO_INT (iter->user_data);

  if ((unit >= 0 && unit < gimp_unit_get_number_of_units ()) ||
      ((unit == GIMP_UNIT_PERCENT && private->has_percent)))
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
        case GIMP_UNIT_STORE_UNIT_SHORT_FORMAT:
          g_value_take_string (value,
                               gimp_unit_format_string (private->short_format,
                                                        unit));
          break;
        case GIMP_UNIT_STORE_UNIT_LONG_FORMAT:
          g_value_take_string (value,
                               gimp_unit_format_string (private->long_format,
                                                        unit));
          break;

        default:
          column -= GIMP_UNIT_STORE_UNIT_COLUMNS;
          if (unit == GIMP_UNIT_PIXEL)
            {
              g_value_set_double (value, private->values[column]);
            }
          else if (private->resolutions[column])
            {
              g_value_set_double (value,
                                  private->values[column] *
                                  gimp_unit_get_factor (unit) /
                                  private->resolutions[column]);
            }
          break;
        }
    }
}

static gboolean
gimp_unit_store_iter_next (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GimpUnit              unit    = GPOINTER_TO_INT (iter->user_data);

  if (unit == GIMP_UNIT_PIXEL && private->has_percent)
    {
      unit = GIMP_UNIT_PERCENT;
    }
  else if (unit == GIMP_UNIT_PERCENT)
    {
      unit = GIMP_UNIT_INCH;
    }
  else if (unit >= 0 && unit < gimp_unit_get_number_of_units () - 1)
    {
      unit++;
    }
  else
    {
      return FALSE;
    }

  iter->user_data = GINT_TO_POINTER (unit);

  return TRUE;
}

static gboolean
gimp_unit_store_iter_children (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter,
                               GtkTreeIter  *parent)
{
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GimpUnit              unit;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  if (private->has_pixels)
    {
      unit = GIMP_UNIT_PIXEL;
    }
  else if (private->has_percent)
    {
      unit = GIMP_UNIT_PERCENT;
    }
  else
    {
      unit = GIMP_UNIT_INCH;
    }

  iter->user_data = GINT_TO_POINTER (unit);

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
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  n_children;

  if (iter)
    return 0;

  n_children = gimp_unit_get_number_of_units ();

  if (! private->has_pixels)
    n_children--;

  if (private->has_percent)
    n_children++;

  return n_children;
}

static gboolean
gimp_unit_store_iter_nth_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent,
                                gint          n)
{
  GimpUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  n_children;

  if (parent)
    return FALSE;

  n_children = gimp_unit_store_iter_n_children (tree_model, NULL);

  if (n >= 0 && n < n_children)
    {
      GimpUnit unit = n;

      if (! private->has_pixels)
        unit++;

      if (private->has_percent)
        {
          unit--;

          if (private->has_pixels)
            {
              if (n == 0)
                unit = GIMP_UNIT_PIXEL;
              else if (n == 1)
                unit = GIMP_UNIT_PERCENT;
            }
          else
            {
              if (n == 0)
                unit = GIMP_UNIT_PERCENT;
            }
        }

      iter->user_data = GINT_TO_POINTER (unit);

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
gimp_unit_store_set_has_pixels (GimpUnitStore *store,
                                gboolean       has_pixels)
{
  GimpUnitStorePrivate *private;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_pixels = has_pixels ? TRUE : FALSE;

  if (has_pixels != private->has_pixels)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_pixels)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_pixels = has_pixels;

      if (has_pixels)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-pixels");
    }
}

gboolean
gimp_unit_store_get_has_pixels (GimpUnitStore *store)
{
  GimpUnitStorePrivate *private;

  g_return_val_if_fail (GIMP_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_pixels;
}

void
gimp_unit_store_set_has_percent (GimpUnitStore *store,
                                 gboolean       has_percent)
{
  GimpUnitStorePrivate *private;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_percent = has_percent ? TRUE : FALSE;

  if (has_percent != private->has_percent)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_percent)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_percent = has_percent;

      if (has_percent)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-percent");
    }
}

gboolean
gimp_unit_store_get_has_percent (GimpUnitStore *store)
{
  GimpUnitStorePrivate *private;

  g_return_val_if_fail (GIMP_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_percent;
}

void
gimp_unit_store_set_pixel_value (GimpUnitStore *store,
                                 gint           index,
                                 gdouble        value)
{
  GimpUnitStorePrivate *private;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->values[index] = value;
}

void
gimp_unit_store_set_pixel_values (GimpUnitStore *store,
                                  gdouble        first_value,
                                  ...)
{
  GimpUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      private->values[i] = first_value;

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble);
    }

  va_end (args);
}

void
gimp_unit_store_set_resolution (GimpUnitStore *store,
                                gint           index,
                                gdouble        resolution)
{
  GimpUnitStorePrivate *private;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->resolutions[index] = resolution;
}

void
gimp_unit_store_set_resolutions  (GimpUnitStore *store,
                                  gdouble        first_resolution,
                                  ...)
{
  GimpUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_resolution);

  for (i = 0; i < private->num_values; )
    {
      private->resolutions[i] = first_resolution;

      if (++i < private->num_values)
        first_resolution = va_arg (args, gdouble);
    }

  va_end (args);
}

gdouble
gimp_unit_store_get_value (GimpUnitStore *store,
                           GimpUnit       unit,
                           gint           index)
{
  GimpUnitStorePrivate *private;
  GtkTreeIter          iter;
  GValue               value = G_VALUE_INIT;

  g_return_val_if_fail (GIMP_IS_UNIT_STORE (store), 0.0);

  private = GET_PRIVATE (store);

  g_return_val_if_fail (index >= 0 && index < private->num_values, 0.0);

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
  GimpUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      if (first_value)
        *first_value = gimp_unit_store_get_value (store, unit, i);

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble *);
    }

  va_end (args);
}

void
_gimp_unit_store_sync_units (GimpUnitStore *store)
{
  GimpUnitStorePrivate *private;
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;

  g_return_if_fail (GIMP_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);
  model   = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint unit;

      gtk_tree_model_get (model, &iter,
                          GIMP_UNIT_STORE_UNIT, &unit,
                          -1);

      if (unit != GIMP_UNIT_PERCENT &&
          unit > private->synced_unit)
        {
          GtkTreePath *path;

          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
    }

  private->synced_unit = gimp_unit_get_number_of_units () - 1;
}
