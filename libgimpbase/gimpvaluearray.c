/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpvaluearray.c ported from GValueArray
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpvaluearray.h"


/**
 * SECTION:value_arrays
 * @short_description: A container structure to maintain an array of
 *     generic values
 * @see_also: #GValue, #GParamSpecValueArray, gimp_param_spec_value_array()
 * @title: Value arrays
 *
 * The prime purpose of a #GimpValueArray is for it to be used as an
 * object property that holds an array of values. A #GimpValueArray wraps
 * an array of #GValue elements in order for it to be used as a boxed
 * type through %GIMP_TYPE_VALUE_ARRAY.
 */


#define GROUP_N_VALUES (1) /* power of 2 !! */


/**
 * GimpValueArray:
 * @n_values: number of values contained in the array
 * @values: array of values
 *
 * A #GimpValueArray contains an array of #GValue elements.
 *
 * Since: GIMP 2.10
 */
struct _GimpValueArray
{
  gint    n_values;
  GValue *values;

  gint    n_prealloced;
  gint    ref_count;
};


G_DEFINE_BOXED_TYPE (GimpValueArray, gimp_value_array,
                     gimp_value_array_ref, gimp_value_array_unref)


/**
 * gimp_value_array_index:
 * @value_array: #GimpValueArray to get a value from
 * @index_: index of the value of interest
 *
 * Return a pointer to the value at @index_ containd in @value_array.
 *
 * Returns: (transfer none): pointer to a value at @index_ in @value_array
 *
 * Since: GIMP 2.10
 */
GValue *
gimp_value_array_index (const GimpValueArray *value_array,
                        gint                  index)
{
  g_return_val_if_fail (value_array != NULL, NULL);
  g_return_val_if_fail (index < value_array->n_values, NULL);

  return value_array->values + index;
}

static inline void
value_array_grow (GimpValueArray *value_array,
		  gint            n_values,
		  gboolean        zero_init)
{
  g_return_if_fail ((guint) n_values >= (guint) value_array->n_values);

  value_array->n_values = n_values;
  if (value_array->n_values > value_array->n_prealloced)
    {
      gint i = value_array->n_prealloced;

      value_array->n_prealloced = (value_array->n_values + GROUP_N_VALUES - 1) & ~(GROUP_N_VALUES - 1);
      value_array->values = g_renew (GValue, value_array->values, value_array->n_prealloced);

      if (!zero_init)
	i = value_array->n_values;

      memset (value_array->values + i, 0,
	      (value_array->n_prealloced - i) * sizeof (value_array->values[0]));
    }
}

static inline void
value_array_shrink (GimpValueArray *value_array)
{
  if (value_array->n_prealloced >= value_array->n_values + GROUP_N_VALUES)
    {
      value_array->n_prealloced = (value_array->n_values + GROUP_N_VALUES - 1) & ~(GROUP_N_VALUES - 1);
      value_array->values = g_renew (GValue, value_array->values, value_array->n_prealloced);
    }
}

/**
 * gimp_value_array_new:
 * @n_prealloced: number of values to preallocate space for
 *
 * Allocate and initialize a new #GimpValueArray, optionally preserve space
 * for @n_prealloced elements. New arrays always contain 0 elements,
 * regardless of the value of @n_prealloced.
 *
 * Returns: a newly allocated #GimpValueArray with 0 values
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_new (gint n_prealloced)
{
  GimpValueArray *value_array = g_slice_new (GimpValueArray);

  value_array->n_values = 0;
  value_array->n_prealloced = 0;
  value_array->values = NULL;
  value_array_grow (value_array, n_prealloced, TRUE);
  value_array->n_values = 0;
  value_array->ref_count = 1;

  return value_array;
}

/**
 * gimp_value_array_ref:
 * @value_array: #GimpValueArray to ref
 *
 * Adds a reference to a #GimpValueArray.
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_ref (GimpValueArray *value_array)
{
  g_return_val_if_fail (value_array != NULL, NULL);

  value_array->ref_count++;

  return value_array;
}

/**
 * gimp_value_array_unref:
 * @value_array: #GimpValueArray to unref
 *
 * Unref a #GimpValueArray. If the reference count drops to zero, the
 * array including its contents are freed.
 *
 * Since: GIMP 2.10
 */
void
gimp_value_array_unref (GimpValueArray *value_array)
{
  g_return_if_fail (value_array != NULL);

  value_array->ref_count--;

  if (value_array->ref_count < 1)
    {
      gint i;

      for (i = 0; i < value_array->n_values; i++)
        {
          GValue *value = value_array->values + i;

          if (G_VALUE_TYPE (value) != 0) /* we allow unset values in the array */
            g_value_unset (value);
        }
      g_free (value_array->values);
      g_slice_free (GimpValueArray, value_array);
    }
}

gint
gimp_value_array_length (const GimpValueArray *value_array)
{
  g_return_val_if_fail (value_array != NULL, 0);

  return value_array->n_values;
}

/**
 * gimp_value_array_prepend:
 * @value_array: #GimpValueArray to add an element to
 * @value: (allow-none): #GValue to copy into #GimpValueArray, or %NULL
 *
 * Insert a copy of @value as first element of @value_array. If @value is
 * %NULL, an uninitialized value is prepended.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_prepend (GimpValueArray *value_array,
                          const GValue   *value)
{
  g_return_val_if_fail (value_array != NULL, NULL);

  return gimp_value_array_insert (value_array, 0, value);
}

/**
 * gimp_value_array_append:
 * @value_array: #GimpValueArray to add an element to
 * @value: (allow-none): #GValue to copy into #GimpValueArray, or %NULL
 *
 * Insert a copy of @value as last element of @value_array. If @value is
 * %NULL, an uninitialized value is appended.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_append (GimpValueArray *value_array,
                         const GValue   *value)
{
  g_return_val_if_fail (value_array != NULL, NULL);

  return gimp_value_array_insert (value_array, value_array->n_values, value);
}

/**
 * gimp_value_array_insert:
 * @value_array: #GimpValueArray to add an element to
 * @index_: insertion position, must be &lt;= value_array-&gt;n_values
 * @value: (allow-none): #GValue to copy into #GimpValueArray, or %NULL
 *
 * Insert a copy of @value at specified position into @value_array. If @value
 * is %NULL, an uninitialized value is inserted.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_insert (GimpValueArray *value_array,
                         gint            index,
                         const GValue   *value)
{
  gint i;

  g_return_val_if_fail (value_array != NULL, NULL);
  g_return_val_if_fail (index <= value_array->n_values, value_array);

  i = value_array->n_values;
  value_array_grow (value_array, value_array->n_values + 1, FALSE);

  if (index + 1 < value_array->n_values)
    g_memmove (value_array->values + index + 1, value_array->values + index,
	       (i - index) * sizeof (value_array->values[0]));

  memset (value_array->values + index, 0, sizeof (value_array->values[0]));

  if (value)
    {
      g_value_init (value_array->values + index, G_VALUE_TYPE (value));
      g_value_copy (value, value_array->values + index);
    }

  return value_array;
}

/**
 * gimp_value_array_remove:
 * @value_array: #GimpValueArray to remove an element from
 * @index_: position of value to remove, which must be less than
 *          <code>value_array-><link
 *          linkend="GimpValueArray.n-values">n_values</link></code>
 *
 * Remove the value at position @index_ from @value_array.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: GIMP 2.10
 */
GimpValueArray *
gimp_value_array_remove (GimpValueArray *value_array,
                         gint            index)
{
  g_return_val_if_fail (value_array != NULL, NULL);
  g_return_val_if_fail (index < value_array->n_values, value_array);

  if (G_VALUE_TYPE (value_array->values + index) != 0)
    g_value_unset (value_array->values + index);

  value_array->n_values--;

  if (index < value_array->n_values)
    g_memmove (value_array->values + index, value_array->values + index + 1,
	       (value_array->n_values - index) * sizeof (value_array->values[0]));

  value_array_shrink (value_array);

  if (value_array->n_prealloced > value_array->n_values)
    memset (value_array->values + value_array->n_values, 0, sizeof (value_array->values[0]));

  return value_array;
}

void
gimp_value_array_truncate (GimpValueArray *value_array,
                           gint            n_values)
{
  gint i;

  g_return_if_fail (value_array != NULL);
  g_return_if_fail (n_values > 0 && n_values <= value_array->n_values);

  for (i = value_array->n_values; i > n_values; i--)
    gimp_value_array_remove (value_array, i - 1);
}
