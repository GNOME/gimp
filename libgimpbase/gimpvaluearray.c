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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpvaluearray.h"


/**
 * SECTION:gimpvaluearray
 * @short_description: A container structure to maintain an array of
 *     generic values
 * @see_also: #GValue, #GParamSpecValueArray, gimp_param_spec_value_array()
 * @title: GimpValueArray
 *
 * The prime purpose of a #GimpValueArray is for it to be used as an
 * object property that holds an array of values. A #GimpValueArray wraps
 * an array of #GValue elements in order for it to be used as a boxed
 * type through %GIMP_TYPE_VALUE_ARRAY.
 */


#define GROUP_N_VALUES (1) /* power of 2 !! */


/**
 * GimpValueArray:
 *
 * A #GimpValueArray contains an array of #GValue elements.
 *
 * Since: 2.10
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
 * @index: index of the value of interest
 *
 * Return a pointer to the value at @index contained in @value_array.
 *
 * Returns: (transfer none): pointer to a value at @index in @value_array
 *
 * Since: 2.10
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
 * Since: 2.10
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
 * Return value: the same @value_array
 *
 * Since: 2.10
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
 * Since: 2.10
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
 * Since: 2.10
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
 * Since: 2.10
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
 * @index: insertion position, must be &lt;= gimp_value_array_length()
 * @value: (allow-none): #GValue to copy into #GimpValueArray, or %NULL
 *
 * Insert a copy of @value at specified position into @value_array. If @value
 * is %NULL, an uninitialized value is inserted.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: 2.10
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
    memmove (value_array->values + index + 1, value_array->values + index,
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
 * @index: position of value to remove, which must be less than
 *         gimp_value_array_length()
 *
 * Remove the value at position @index from @value_array.
 *
 * Returns: (transfer none): the #GimpValueArray passed in as @value_array
 *
 * Since: 2.10
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
    memmove (value_array->values + index, value_array->values + index + 1,
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


/*
 * GIMP_TYPE_PARAM_VALUE_ARRAY
 */

static void       gimp_param_value_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_value_array_init        (GParamSpec      *pspec);
static void       gimp_param_value_array_finalize    (GParamSpec      *pspec);
static void       gimp_param_value_array_set_default (GParamSpec      *pspec,
                                                      GValue          *value);
static gboolean   gimp_param_value_array_validate    (GParamSpec      *pspec,
                                                      GValue          *value);
static gint       gimp_param_value_array_values_cmp  (GParamSpec      *pspec,
                                                      const GValue    *value1,
                                                      const GValue    *value2);

GType
gimp_param_value_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_value_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecValueArray),
        0,
        (GInstanceInitFunc) gimp_param_value_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamValueArray", &info, 0);
    }

  return type;
}


static void
gimp_param_value_array_class_init (GParamSpecClass *klass)
{
  klass->value_type        = GIMP_TYPE_VALUE_ARRAY;
  klass->finalize          = gimp_param_value_array_finalize;
  klass->value_set_default = gimp_param_value_array_set_default;
  klass->value_validate    = gimp_param_value_array_validate;
  klass->values_cmp        = gimp_param_value_array_values_cmp;
}

static void
gimp_param_value_array_init (GParamSpec *pspec)
{
  GimpParamSpecValueArray *aspec = GIMP_PARAM_SPEC_VALUE_ARRAY (pspec);

  aspec->element_spec = NULL;
  aspec->fixed_n_elements = 0; /* disable */
}

static inline guint
gimp_value_array_ensure_size (GimpValueArray *value_array,
                              guint           fixed_n_elements)
{
  guint changed = 0;

  if (fixed_n_elements)
    {
      while (gimp_value_array_length (value_array) < fixed_n_elements)
        {
          gimp_value_array_append (value_array, NULL);
          changed++;
        }

      while (gimp_value_array_length (value_array) > fixed_n_elements)
        {
          gimp_value_array_remove (value_array,
                                   gimp_value_array_length (value_array) - 1);
          changed++;
        }
    }

  return changed;
}

static void
gimp_param_value_array_finalize (GParamSpec *pspec)
{
  GimpParamSpecValueArray *aspec = GIMP_PARAM_SPEC_VALUE_ARRAY (pspec);
  GParamSpecClass         *parent_class;

  parent_class = g_type_class_peek (g_type_parent (GIMP_TYPE_PARAM_VALUE_ARRAY));

  g_clear_pointer (&aspec->element_spec, g_param_spec_unref);

  parent_class->finalize (pspec);
}

static void
gimp_param_value_array_set_default (GParamSpec *pspec,
                                    GValue     *value)
{
  GimpParamSpecValueArray *aspec = GIMP_PARAM_SPEC_VALUE_ARRAY (pspec);

  if (! value->data[0].v_pointer && aspec->fixed_n_elements)
    value->data[0].v_pointer = gimp_value_array_new (aspec->fixed_n_elements);

  if (value->data[0].v_pointer)
    {
      /* g_value_reset (value);  already done */
      gimp_value_array_ensure_size (value->data[0].v_pointer,
                                    aspec->fixed_n_elements);
    }
}

static gboolean
gimp_param_value_array_validate (GParamSpec *pspec,
                                 GValue     *value)
{
  GimpParamSpecValueArray *aspec       = GIMP_PARAM_SPEC_VALUE_ARRAY (pspec);
  GimpValueArray          *value_array = value->data[0].v_pointer;
  guint                    changed     = 0;

  if (! value->data[0].v_pointer && aspec->fixed_n_elements)
    value->data[0].v_pointer = gimp_value_array_new (aspec->fixed_n_elements);

  if (value->data[0].v_pointer)
    {
      /* ensure array size validity */
      changed += gimp_value_array_ensure_size (value_array,
                                               aspec->fixed_n_elements);

      /* ensure array values validity against a present element spec */
      if (aspec->element_spec)
        {
          GParamSpec *element_spec = aspec->element_spec;
          gint        length       = gimp_value_array_length (value_array);
          gint        i;

          for (i = 0; i < length; i++)
            {
              GValue *element = gimp_value_array_index (value_array, i);

              /* need to fixup value type, or ensure that the array
               * value is initialized at all
               */
              if (! g_value_type_compatible (G_VALUE_TYPE (element),
                                             G_PARAM_SPEC_VALUE_TYPE (element_spec)))
                {
                  if (G_VALUE_TYPE (element) != 0)
                    g_value_unset (element);

                  g_value_init (element, G_PARAM_SPEC_VALUE_TYPE (element_spec));
                  g_param_value_set_default (element_spec, element);
                  changed++;
                }

              /* validate array value against element_spec */
              changed += g_param_value_validate (element_spec, element);
            }
        }
    }

  return changed;
}

static gint
gimp_param_value_array_values_cmp (GParamSpec   *pspec,
                                   const GValue *value1,
                                   const GValue *value2)
{
  GimpParamSpecValueArray *aspec        = GIMP_PARAM_SPEC_VALUE_ARRAY (pspec);
  GimpValueArray          *value_array1 = value1->data[0].v_pointer;
  GimpValueArray          *value_array2 = value2->data[0].v_pointer;
  gint                     length1;
  gint                     length2;

  if (!value_array1 || !value_array2)
    return value_array2 ? -1 : value_array1 != value_array2;

  length1 = gimp_value_array_length (value_array1);
  length2 = gimp_value_array_length (value_array2);

  if (length1 != length2)
    {
      return length1 < length2 ? -1 : 1;
    }
  else if (! aspec->element_spec)
    {
      /* we need an element specification for comparisons, so there's
       * not much to compare here, try to at least provide stable
       * lesser/greater result
       */
      return length1 < length2 ? -1 : length1 > length2;
    }
  else /* length1 == length2 */
    {
      guint i;

      for (i = 0; i < length1; i++)
        {
          GValue *element1 = gimp_value_array_index (value_array1, i);
          GValue *element2 = gimp_value_array_index (value_array2, i);
          gint    cmp;

          /* need corresponding element types, provide stable result
           * otherwise
           */
          if (G_VALUE_TYPE (element1) != G_VALUE_TYPE (element2))
            return G_VALUE_TYPE (element1) < G_VALUE_TYPE (element2) ? -1 : 1;

          cmp = g_param_values_cmp (aspec->element_spec, element1, element2);
          if (cmp)
            return cmp;
        }

      return 0;
    }
}

GParamSpec *
gimp_param_spec_value_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamSpec  *element_spec,
                             GParamFlags  flags)
{
  GimpParamSpecValueArray *aspec;

  if (element_spec)
    g_return_val_if_fail (G_IS_PARAM_SPEC (element_spec), NULL);

  aspec = g_param_spec_internal (GIMP_TYPE_PARAM_VALUE_ARRAY,
                                 name,
                                 nick,
                                 blurb,
                                 flags);
  if (element_spec)
    {
      aspec->element_spec = g_param_spec_ref (element_spec);
      g_param_spec_sink (element_spec);
    }

  return G_PARAM_SPEC (aspec);
}
