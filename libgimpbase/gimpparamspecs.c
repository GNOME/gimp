/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs.c
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

#include <gio/gio.h>

#include "gimpbase.h"


/*
 * GIMP_TYPE_ARRAY
 */

GimpArray *
gimp_array_new (const guint8 *data,
                gsize         length,
                gboolean      static_data)
{
  GimpArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (GimpArray);

  array->data        = static_data ? (guint8 *) data : g_memdup (data, length);
  array->length      = length;
  array->static_data = static_data;

  return array;
}

GimpArray *
gimp_array_copy (const GimpArray *array)
{
  if (array)
    return gimp_array_new (array->data, array->length, FALSE);

  return NULL;
}

void
gimp_array_free (GimpArray *array)
{
  if (array)
    {
      if (! array->static_data)
        g_free (array->data);

      g_slice_free (GimpArray, array);
    }
}

GType
gimp_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_ARRAY
 */

static void       gimp_param_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_array_validate    (GParamSpec      *pspec,
                                                GValue          *value);
static gint       gimp_param_array_values_cmp  (GParamSpec      *pspec,
                                                const GValue    *value1,
                                                const GValue    *value2);

GType
gimp_param_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamArray", &info, 0);
    }

  return type;
}

static void
gimp_param_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_ARRAY;
  klass->value_validate = gimp_param_array_validate;
  klass->values_cmp     = gimp_param_array_values_cmp;
}

static void
gimp_param_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_array_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    {
      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

static gint
gimp_param_array_values_cmp (GParamSpec   *pspec,
                             const GValue *value1,
                             const GValue *value2)
{
  GimpArray *array1 = value1->data[0].v_pointer;
  GimpArray *array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (array1->length < array2->length)
    return -1;
  else if (array1->length > array2->length)
    return 1;

  return 0;
}

/**
 * gimp_param_spec_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecArray specifying a
 * #GIMP_TYPE_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_array (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

static const guint8 *
gimp_value_get_array (const GValue *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

static guint8 *
gimp_value_dup_array (const GValue *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    return g_memdup (array->data, array->length);

  return NULL;
}

static void
gimp_value_set_array (GValue       *value,
                      const guint8 *data,
                      gsize         length)
{
  GimpArray *array = gimp_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

static void
gimp_value_set_static_array (GValue       *value,
                             const guint8 *data,
                             gsize         length)
{
  GimpArray *array = gimp_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

static void
gimp_value_take_array (GValue *value,
                       guint8 *data,
                       gsize   length)
{
  GimpArray *array = gimp_array_new (data, length, TRUE);

  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * GIMP_TYPE_UINT8_ARRAY
 */

GType
gimp_uint8_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpUInt8Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_UINT8_ARRAY
 */

static void   gimp_param_uint8_array_class_init (GParamSpecClass *klass);
static void   gimp_param_uint8_array_init       (GParamSpec      *pspec);

GType
gimp_param_uint8_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_uint8_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_uint8_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamUInt8Array", &info, 0);
    }

  return type;
}

static void
gimp_param_uint8_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_UINT8_ARRAY;
}

static void
gimp_param_uint8_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_uint8_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecUInt8Array specifying a
 * #GIMP_TYPE_UINT8_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecUInt8Array.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_uint8_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_UINT8_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const guint8 *
gimp_value_get_uint8_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_UINT8_ARRAY (value), NULL);

  return gimp_value_get_array (value);
}

guint8 *
gimp_value_dup_uint8_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_UINT8_ARRAY (value), NULL);

  return gimp_value_dup_array (value);
}

void
gimp_value_set_uint8_array (GValue       *value,
                            const guint8 *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_UINT8_ARRAY (value));

  gimp_value_set_array (value, data, length);
}

void
gimp_value_set_static_uint8_array (GValue       *value,
                                   const guint8 *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_UINT8_ARRAY (value));

  gimp_value_set_static_array (value, data, length);
}

void
gimp_value_take_uint8_array (GValue *value,
                             guint8 *data,
                             gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_UINT8_ARRAY (value));

  gimp_value_take_array (value, data, length);
}


/*
 * GIMP_TYPE_INT16_ARRAY
 */

GType
gimp_int16_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpInt16Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT16_ARRAY
 */

static void   gimp_param_int16_array_class_init (GParamSpecClass *klass);
static void   gimp_param_int16_array_init       (GParamSpec      *pspec);

GType
gimp_param_int16_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int16_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_int16_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamInt16Array", &info, 0);
    }

  return type;
}

static void
gimp_param_int16_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT16_ARRAY;
}

static void
gimp_param_int16_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_int16_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecInt16Array specifying a
 * #GIMP_TYPE_INT16_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecInt16Array.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_int16_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT16_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gint16 *
gimp_value_get_int16_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value), NULL);

  return (const gint16 *) gimp_value_get_array (value);
}

gint16 *
gimp_value_dup_int16_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value), NULL);

  return (gint16 *) gimp_value_dup_array (value);
}

void
gimp_value_set_int16_array (GValue       *value,
                            const gint16 *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint16));
}

void
gimp_value_set_static_int16_array (GValue       *value,
                                   const gint16 *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint16));
}

void
gimp_value_take_int16_array (GValue *value,
                             gint16 *data,
                             gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint16));
}


/*
 * GIMP_TYPE_INT32_ARRAY
 */

GType
gimp_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpInt32Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT32_ARRAY
 */

static void   gimp_param_int32_array_class_init (GParamSpecClass *klass);
static void   gimp_param_int32_array_init       (GParamSpec      *pspec);

GType
gimp_param_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int32_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_int32_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamInt32Array", &info, 0);
    }

  return type;
}

static void
gimp_param_int32_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT32_ARRAY;
}

static void
gimp_param_int32_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_int32_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecInt32Array specifying a
 * #GIMP_TYPE_INT32_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecInt32Array.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_int32_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT32_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gint32 *
gimp_value_get_int32_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (const gint32 *) gimp_value_get_array (value);
}

gint32 *
gimp_value_dup_int32_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (gint32 *) gimp_value_dup_array (value);
}

void
gimp_value_set_int32_array (GValue       *value,
                            const gint32 *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint32));
}

void
gimp_value_set_static_int32_array (GValue       *value,
                                   const gint32 *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint32));
}

void
gimp_value_take_int32_array (GValue *value,
                             gint32 *data,
                             gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint32));
}


/*
 * GIMP_TYPE_FLOAT_ARRAY
 */

GType
gimp_float_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpFloatArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_FLOAT_ARRAY
 */

static void   gimp_param_float_array_class_init (GParamSpecClass *klass);
static void   gimp_param_float_array_init       (GParamSpec      *pspec);

GType
gimp_param_float_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_float_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_float_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamFloatArray", &info, 0);
    }

  return type;
}

static void
gimp_param_float_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_FLOAT_ARRAY;
}

static void
gimp_param_float_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_float_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecFloatArray specifying a
 * #GIMP_TYPE_FLOAT_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecFloatArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_float_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_FLOAT_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gdouble *
gimp_value_get_float_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (const gdouble *) gimp_value_get_array (value);
}

gdouble *
gimp_value_dup_float_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (gdouble *) gimp_value_dup_array (value);
}

void
gimp_value_set_float_array (GValue        *value,
                            const gdouble *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

void
gimp_value_set_static_float_array (GValue        *value,
                                   const gdouble *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

void
gimp_value_take_float_array (GValue  *value,
                             gdouble *data,
                             gsize    length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gdouble));
}


/*
 * GIMP_TYPE_STRING_ARRAY
 */

/**
 * gimp_string_array_new:
 * @data: (array length=length) (transfer none): an array of strings.
 * @length: the length of @data.
 * @static_data: whether the strings in @data are static strings rather
 *               than allocated.
 *
 * Creates a new #GimpStringArray containing string data, of size @length.
 *
 * If @static_data is %TRUE, @data is used as-is.
 *
 * If @static_data is %FALSE, the string and array will be re-allocated,
 * hence you are expected to free your input data after.
 *
 * Returns: (transfer full): a new #GimpStringArray.
 */
GimpStringArray *
gimp_string_array_new (const gchar **data,
                       gsize         length,
                       gboolean      static_data)
{
  GimpStringArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (GimpStringArray);

  if (! static_data && data)
    {
      gchar **tmp = g_new0 (gchar *, length + 1);
      gint    i;

      for (i = 0; i < length; i++)
        tmp[i] = g_strdup (data[i]);

      array->data = tmp;
    }
  else
    {
      array->data = (gchar **) data;
    }

  array->length      = length;
  array->static_data = static_data;

  return array;
}

/**
 * gimp_string_array_copy:
 * @array: an original #GimpStringArray of strings.
 *
 * Creates a new #GimpStringArray containing a deep copy of @array.
 *
 * Returns: (transfer full): a new #GimpStringArray.
 **/
GimpStringArray *
gimp_string_array_copy (const GimpStringArray *array)
{
  if (array)
    return gimp_string_array_new ((const gchar **) array->data,
                                  array->length, FALSE);

  return NULL;
}

void
gimp_string_array_free (GimpStringArray *array)
{
  if (array)
    {
      if (! array->static_data)
        {
          gchar **tmp = array->data;
          gint    i;

          for (i = 0; i < array->length; i++)
            g_free (tmp[i]);

          g_free (array->data);
        }

      g_slice_free (GimpStringArray, array);
    }
}

GType
gimp_string_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpStringArray",
                                         (GBoxedCopyFunc) gimp_string_array_copy,
                                         (GBoxedFreeFunc) gimp_string_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_STRING_ARRAY
 */

static void       gimp_param_string_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_string_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_string_array_validate    (GParamSpec      *pspec,
                                                       GValue          *value);
static gint       gimp_param_string_array_values_cmp  (GParamSpec      *pspec,
                                                       const GValue    *value1,
                                                       const GValue    *value2);

GType
gimp_param_string_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_string_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_string_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamStringArray", &info, 0);
    }

  return type;
}

static void
gimp_param_string_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_STRING_ARRAY;
  klass->value_validate = gimp_param_string_array_validate;
  klass->values_cmp     = gimp_param_string_array_values_cmp;
}

static void
gimp_param_string_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_string_array_validate (GParamSpec *pspec,
                                  GValue     *value)
{
  GimpStringArray *array = value->data[0].v_pointer;

  if (array)
    {
      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

static gint
gimp_param_string_array_values_cmp (GParamSpec   *pspec,
                                    const GValue *value1,
                                    const GValue *value2)
{
  GimpStringArray *array1 = value1->data[0].v_pointer;
  GimpStringArray *array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (array1->length < array2->length)
    return -1;
  else if (array1->length > array2->length)
    return 1;

  return 0;
}

/**
 * gimp_param_spec_string_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecStringArray specifying a
 * #GIMP_TYPE_STRING_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecStringArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_string_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GParamFlags  flags)
{
  GimpParamSpecStringArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_STRING_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

/**
 * gimp_value_get_string_array:
 * @value: a #GValue holding a string #GimpStringArray.
 *
 * Returns: (transfer none) (array zero-terminated=1): the internal
 *          array of strings.
 */
const gchar **
gimp_value_get_string_array (const GValue *value)
{
  GimpStringArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    return (const gchar **) array->data;

  return NULL;
}

/**
 * gimp_value_dup_string_array:
 * @value: a #GValue holding a string #GimpStringArray.
 *
 * Returns: (transfer full) (array zero-terminated=1): a deep copy of
 *          the array of strings.
 */
gchar **
gimp_value_dup_string_array (const GValue *value)
{
  GimpStringArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    {
      gchar **ret = g_memdup (array->data, (array->length + 1) * sizeof (gchar *));
      gint    i;

      for (i = 0; i < array->length; i++)
        ret[i] = g_strdup (ret[i]);

      return ret;
    }

  return NULL;
}

void
gimp_value_set_string_array (GValue       *value,
                             const gchar **data,
                             gsize         length)
{
  GimpStringArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

void
gimp_value_set_static_string_array (GValue       *value,
                                    const gchar **data,
                                    gsize         length)
{
  GimpStringArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

void
gimp_value_take_string_array (GValue  *value,
                              gchar  **data,
                              gsize    length)
{
  GimpStringArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new ((const gchar **) data, length, TRUE);
  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * GIMP_TYPE_RGB_ARRAY
 */

GType
gimp_rgb_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpRGBArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_RGB_ARRAY
 */

static void  gimp_param_rgb_array_class_init (GParamSpecClass *klass);
static void  gimp_param_rgb_array_init       (GParamSpec      *pspec);

GType
gimp_param_rgb_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_rgb_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_rgb_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamRGBArray", &info, 0);
    }

  return type;
}

static void
gimp_param_rgb_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_RGB_ARRAY;
}

static void
gimp_param_rgb_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_rgb_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecRGBArray specifying a
 * #GIMP_TYPE_RGB_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecRGBArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_rgb_array (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           GParamFlags  flags)
{
  GimpParamSpecRGBArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_RGB_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const GimpRGB *
gimp_value_get_rgb_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_RGB_ARRAY (value), NULL);

  return (const GimpRGB *) gimp_value_get_array (value);
}

GimpRGB *
gimp_value_dup_rgb_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_RGB_ARRAY (value), NULL);

  return (GimpRGB *) gimp_value_dup_array (value);
}

void
gimp_value_set_rgb_array (GValue        *value,
                          const GimpRGB *data,
                          gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_RGB_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (GimpRGB));
}

void
gimp_value_set_static_rgb_array (GValue        *value,
                                 const GimpRGB *data,
                                 gsize          length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_RGB_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (GimpRGB));
}

void
gimp_value_take_rgb_array (GValue  *value,
                           GimpRGB *data,
                           gsize    length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_RGB_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (GimpRGB));
}
