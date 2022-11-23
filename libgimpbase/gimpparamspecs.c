/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaparamspecs.c
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

#include "ligmabase.h"


/*
 * LIGMA_TYPE_ARRAY
 */

/**
 * ligma_array_new:
 * @data: (array length=length):
 * @length:
 * @static_data:
 */
LigmaArray *
ligma_array_new (const guint8 *data,
                gsize         length,
                gboolean      static_data)
{
  LigmaArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (LigmaArray);

  array->data        = static_data ? (guint8 *) data : g_memdup2 (data, length);
  array->length      = length;
  array->static_data = static_data;

  return array;
}

LigmaArray *
ligma_array_copy (const LigmaArray *array)
{
  if (array)
    return ligma_array_new (array->data, array->length, FALSE);

  return NULL;
}

void
ligma_array_free (LigmaArray *array)
{
  if (array)
    {
      if (! array->static_data)
        g_free (array->data);

      g_slice_free (LigmaArray, array);
    }
}

G_DEFINE_BOXED_TYPE (LigmaArray, ligma_array, ligma_array_copy, ligma_array_free)


/*
 * LIGMA_TYPE_PARAM_ARRAY
 */

static void       ligma_param_array_class_init  (GParamSpecClass *klass);
static void       ligma_param_array_init        (GParamSpec      *pspec);
static gboolean   ligma_param_array_validate    (GParamSpec      *pspec,
                                                GValue          *value);
static gint       ligma_param_array_values_cmp  (GParamSpec      *pspec,
                                                const GValue    *value1,
                                                const GValue    *value2);

GType
ligma_param_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecArray),
        0,
        (GInstanceInitFunc) ligma_param_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "LigmaParamArray", &info, 0);
    }

  return type;
}

static void
ligma_param_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = LIGMA_TYPE_ARRAY;
  klass->value_validate = ligma_param_array_validate;
  klass->values_cmp     = ligma_param_array_values_cmp;
}

static void
ligma_param_array_init (GParamSpec *pspec)
{
}

static gboolean
ligma_param_array_validate (GParamSpec *pspec,
                           GValue     *value)
{
  LigmaArray *array = value->data[0].v_pointer;

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
ligma_param_array_values_cmp (GParamSpec   *pspec,
                             const GValue *value1,
                             const GValue *value2)
{
  LigmaArray *array1 = value1->data[0].v_pointer;
  LigmaArray *array2 = value2->data[0].v_pointer;

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
 * ligma_param_spec_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecArray specifying a
 * #LIGMA_TYPE_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecArray.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_array (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  LigmaParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

static const guint8 *
ligma_value_get_array (const GValue *value)
{
  LigmaArray *array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

static guint8 *
ligma_value_dup_array (const GValue *value)
{
  LigmaArray *array = value->data[0].v_pointer;

  if (array)
    return g_memdup2 (array->data, array->length);

  return NULL;
}

static void
ligma_value_set_array (GValue       *value,
                      const guint8 *data,
                      gsize         length)
{
  LigmaArray *array = ligma_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

static void
ligma_value_set_static_array (GValue       *value,
                             const guint8 *data,
                             gsize         length)
{
  LigmaArray *array = ligma_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

static void
ligma_value_take_array (GValue *value,
                       guint8 *data,
                       gsize   length)
{
  LigmaArray *array = ligma_array_new (data, length, TRUE);

  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * LIGMA_TYPE_UINT8_ARRAY
 */

typedef LigmaArray LigmaUint8Array;
G_DEFINE_BOXED_TYPE (LigmaUint8Array, ligma_uint8_array, ligma_array_copy, ligma_array_free)

/*
 * LIGMA_TYPE_PARAM_UINT8_ARRAY
 */

static void   ligma_param_uint8_array_class_init (GParamSpecClass *klass);
static void   ligma_param_uint8_array_init       (GParamSpec      *pspec);

GType
ligma_param_uint8_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_uint8_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecUInt8Array),
        0,
        (GInstanceInitFunc) ligma_param_uint8_array_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_ARRAY,
                                     "LigmaParamUInt8Array", &info, 0);
    }

  return type;
}

static void
ligma_param_uint8_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_UINT8_ARRAY;
}

static void
ligma_param_uint8_array_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_uint8_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecUInt8Array specifying a
 * #LIGMA_TYPE_UINT8_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecUInt8Array.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_uint8_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  LigmaParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_UINT8_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

/**
 * ligma_value_get_uint8_array:
 * @value: A valid value of type %LIGMA_TYPE_UINT8_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_UINT8_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const guint8 *
ligma_value_get_uint8_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value), NULL);

  return ligma_value_get_array (value);
}

/**
 * ligma_value_dup_uint8_array:
 * @value: A valid value of type %LIGMA_TYPE_UINT8_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_UINT8_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
guint8 *
ligma_value_dup_uint8_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value), NULL);

  return ligma_value_dup_array (value);
}

/**
 * ligma_value_set_uint8_array:
 * @value: A valid value of type %LIGMA_TYPE_UINT8_ARRAY
 * @data: (array length=length): A #guint8 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
ligma_value_set_uint8_array (GValue       *value,
                            const guint8 *data,
                            gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value));

  ligma_value_set_array (value, data, length);
}

/**
 * ligma_value_set_static_uint8_array:
 * @value: A valid value of type %LIGMA_TYPE_UINT8_ARRAY
 * @data: (array length=length): A #guint8 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
ligma_value_set_static_uint8_array (GValue       *value,
                                   const guint8 *data,
                                   gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value));

  ligma_value_set_static_array (value, data, length);
}

/**
 * ligma_value_take_uint8_array:
 * @value: A valid value of type %LIGMA_TYPE_UINT8_ARRAY
 * @data: (transfer full) (array length=length): A #guint8 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
ligma_value_take_uint8_array (GValue *value,
                             guint8 *data,
                             gsize   length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_UINT8_ARRAY (value));

  ligma_value_take_array (value, data, length);
}


/*
 * LIGMA_TYPE_INT32_ARRAY
 */

typedef LigmaArray LigmaInt32Array;
G_DEFINE_BOXED_TYPE (LigmaInt32Array, ligma_int32_array, ligma_array_copy, ligma_array_free)

/*
 * LIGMA_TYPE_PARAM_INT32_ARRAY
 */

static void   ligma_param_int32_array_class_init (GParamSpecClass *klass);
static void   ligma_param_int32_array_init       (GParamSpec      *pspec);

GType
ligma_param_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_int32_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecInt32Array),
        0,
        (GInstanceInitFunc) ligma_param_int32_array_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_ARRAY,
                                     "LigmaParamInt32Array", &info, 0);
    }

  return type;
}

static void
ligma_param_int32_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_INT32_ARRAY;
}

static void
ligma_param_int32_array_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_int32_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecInt32Array specifying a
 * #LIGMA_TYPE_INT32_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecInt32Array.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_int32_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  LigmaParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_INT32_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

/**
 * ligma_value_get_int32_array:
 * @value: A valid value of type %LIGMA_TYPE_INT32_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const gint32 *
ligma_value_get_int32_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (const gint32 *) ligma_value_get_array (value);
}

/**
 * ligma_value_dup_int32_array:
 * @value: A valid value of type %LIGMA_TYPE_INT32_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
gint32 *
ligma_value_dup_int32_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (gint32 *) ligma_value_dup_array (value);
}

/**
 * ligma_value_set_int32_array:
 * @value: A valid value of type %LIGMA_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
ligma_value_set_int32_array (GValue       *value,
                            const gint32 *data,
                            gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_INT32_ARRAY (value));

  ligma_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint32));
}

/**
 * ligma_value_set_static_int32_array:
 * @value: A valid value of type %LIGMA_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
ligma_value_set_static_int32_array (GValue       *value,
                                   const gint32 *data,
                                   gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_INT32_ARRAY (value));

  ligma_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint32));
}

/**
 * ligma_value_take_int32_array:
 * @value: A valid value of type %LIGMA_TYPE_int32_ARRAY
 * @data: (transfer full) (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
ligma_value_take_int32_array (GValue *value,
                             gint32 *data,
                             gsize   length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_INT32_ARRAY (value));

  ligma_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint32));
}


/*
 * LIGMA_TYPE_FLOAT_ARRAY
 */

typedef LigmaArray LigmaFloatArray;
G_DEFINE_BOXED_TYPE (LigmaFloatArray, ligma_float_array, ligma_array_copy, ligma_array_free)

/*
 * LIGMA_TYPE_PARAM_FLOAT_ARRAY
 */

static void   ligma_param_float_array_class_init (GParamSpecClass *klass);
static void   ligma_param_float_array_init       (GParamSpec      *pspec);

GType
ligma_param_float_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_float_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecFloatArray),
        0,
        (GInstanceInitFunc) ligma_param_float_array_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_ARRAY,
                                     "LigmaParamFloatArray", &info, 0);
    }

  return type;
}

static void
ligma_param_float_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_FLOAT_ARRAY;
}

static void
ligma_param_float_array_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_float_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecFloatArray specifying a
 * #LIGMA_TYPE_FLOAT_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecFloatArray.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_float_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  LigmaParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_FLOAT_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

/**
 * ligma_value_get_float_array:
 * @value: A valid value of type %LIGMA_TYPE_FLOAT_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_FLOAT_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const gdouble *
ligma_value_get_float_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (const gdouble *) ligma_value_get_array (value);
}

/**
 * ligma_value_dup_float_array:
 * @value: A valid value of type %LIGMA_TYPE_FLOAT_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_FLOAT_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
gdouble *
ligma_value_dup_float_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (gdouble *) ligma_value_dup_array (value);
}

/**
 * ligma_value_set_float_array:
 * @value: A valid value of type %LIGMA_TYPE_FLOAT_ARRAY
 * @data: (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
ligma_value_set_float_array (GValue        *value,
                            const gdouble *data,
                            gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value));

  ligma_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

/**
 * ligma_value_set_static_float_array:
 * @value: A valid value of type %LIGMA_TYPE_FLOAT_ARRAY
 * @data: (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
ligma_value_set_static_float_array (GValue        *value,
                                   const gdouble *data,
                                   gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value));

  ligma_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

/**
 * ligma_value_take_float_array:
 * @value: A valid value of type %LIGMA_TYPE_FLOAT_ARRAY
 * @data: (transfer full) (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
ligma_value_take_float_array (GValue  *value,
                             gdouble *data,
                             gsize    length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value));

  ligma_value_take_array (value, (guint8 *) data,
                         length * sizeof (gdouble));
}

/*
 * LIGMA_TYPE_RGB_ARRAY
 */

typedef LigmaArray LigmaRGBArray;
G_DEFINE_BOXED_TYPE (LigmaRGBArray, ligma_rgb_array, ligma_array_copy, ligma_array_free)


/*
 * LIGMA_TYPE_PARAM_RGB_ARRAY
 */

static void  ligma_param_rgb_array_class_init (GParamSpecClass *klass);
static void  ligma_param_rgb_array_init       (GParamSpec      *pspec);

GType
ligma_param_rgb_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_rgb_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecRGBArray),
        0,
        (GInstanceInitFunc) ligma_param_rgb_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "LigmaParamRGBArray", &info, 0);
    }

  return type;
}

static void
ligma_param_rgb_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_RGB_ARRAY;
}

static void
ligma_param_rgb_array_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_rgb_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecRGBArray specifying a
 * #LIGMA_TYPE_RGB_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecRGBArray.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_rgb_array (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           GParamFlags  flags)
{
  LigmaParamSpecRGBArray *array_spec;

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_RGB_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

/**
 * ligma_value_get_rgb_array:
 * @value: A valid value of type %LIGMA_TYPE_RGB_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_RGB_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const LigmaRGB *
ligma_value_get_rgb_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_RGB_ARRAY (value), NULL);

  return (const LigmaRGB *) ligma_value_get_array (value);
}

/**
 * ligma_value_dup_rgb_array:
 * @value: A valid value of type %LIGMA_TYPE_RGB_ARRAY
 *
 * Gets the contents of a %LIGMA_TYPE_RGB_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
LigmaRGB *
ligma_value_dup_rgb_array (const GValue *value)
{
  g_return_val_if_fail (LIGMA_VALUE_HOLDS_RGB_ARRAY (value), NULL);

  return (LigmaRGB *) ligma_value_dup_array (value);
}

/**
 * ligma_value_set_rgb_array:
 * @value: A valid value of type %LIGMA_TYPE_RGB_ARRAY
 * @data: (array length=length): A #LigmaRGB array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
ligma_value_set_rgb_array (GValue        *value,
                          const LigmaRGB *data,
                          gsize         length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_RGB_ARRAY (value));

  ligma_value_set_array (value, (const guint8 *) data,
                        length * sizeof (LigmaRGB));
}

/**
 * ligma_value_set_static_rgb_array:
 * @value: A valid value of type %LIGMA_TYPE_RGB_ARRAY
 * @data: (array length=length): A #LigmaRGB array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
ligma_value_set_static_rgb_array (GValue        *value,
                                 const LigmaRGB *data,
                                 gsize          length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_RGB_ARRAY (value));

  ligma_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (LigmaRGB));
}

/**
 * ligma_value_take_rgb_array:
 * @value: A valid value of type %LIGMA_TYPE_RGB_ARRAY
 * @data: (transfer full) (array length=length): A #LigmaRGB array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
ligma_value_take_rgb_array (GValue  *value,
                           LigmaRGB *data,
                           gsize    length)
{
  g_return_if_fail (LIGMA_VALUE_HOLDS_RGB_ARRAY (value));

  ligma_value_take_array (value, (guint8 *) data,
                         length * sizeof (LigmaRGB));
}


/*
 * LIGMA_TYPE_OBJECT_ARRAY
 */

/**
 * ligma_object_array_new:
 * @data:        (array length=length) (transfer none): an array of objects.
 * @object_type: the array will hold objects of this type
 * @length:      the length of @data.
 * @static_data: whether the objects in @data are static objects and don't
 *               need to be copied.
 *
 * Creates a new #LigmaObjectArray containing object pointers, of size @length.
 *
 * If @static_data is %TRUE, @data is used as-is.
 *
 * If @static_data is %FALSE, the object and array will be re-allocated,
 * hence you are expected to free your input data after.
 *
 * Returns: (transfer full): a new #LigmaObjectArray.
 */
LigmaObjectArray *
ligma_object_array_new (GType     object_type,
                       GObject **data,
                       gsize     length,
                       gboolean  static_data)
{
  LigmaObjectArray *array;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (LigmaObjectArray);

  array->object_type = object_type;

  if (! static_data && data)
    {
      GObject **tmp = g_new0 (GObject *, length);
      gsize     i;

      for (i = 0; i < length; i++)
        tmp[i] = g_object_ref (data[i]);

      array->data = tmp;
    }
  else
    {
      array->data = data;
    }

  array->length      = length;
  array->static_data = static_data;

  return array;
}

/**
 * ligma_object_array_copy:
 * @array: an original #LigmaObjectArray of objects.
 *
 * Creates a new #LigmaObjectArray containing a deep copy of @array.
 *
 * Returns: (transfer full): a new #LigmaObjectArray.
 **/
LigmaObjectArray *
ligma_object_array_copy (const LigmaObjectArray *array)
{
  if (array)
    return ligma_object_array_new (array->object_type,
                                  array->data,
                                  array->length, FALSE);

  return NULL;
}

void
ligma_object_array_free (LigmaObjectArray *array)
{
  if (array)
    {
      if (! array->static_data)
        {
          GObject **tmp = array->data;
          gsize     i;

          for (i = 0; i < array->length; i++)
            g_object_unref (tmp[i]);

          g_free (array->data);
        }

      g_slice_free (LigmaObjectArray, array);
    }
}

G_DEFINE_BOXED_TYPE (LigmaObjectArray, ligma_object_array, ligma_object_array_copy, ligma_object_array_free)


/*
 * LIGMA_TYPE_PARAM_OBJECT_ARRAY
 */

static void       ligma_param_object_array_class_init  (GParamSpecClass *klass);
static void       ligma_param_object_array_init        (GParamSpec      *pspec);
static gboolean   ligma_param_object_array_validate    (GParamSpec      *pspec,
                                                       GValue          *value);
static gint       ligma_param_object_array_values_cmp  (GParamSpec      *pspec,
                                                       const GValue    *value1,
                                                       const GValue    *value2);

GType
ligma_param_object_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_object_array_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecObjectArray),
        0,
        (GInstanceInitFunc) ligma_param_object_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "LigmaParamObjectArray", &info, 0);
    }

  return type;
}

static void
ligma_param_object_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = LIGMA_TYPE_OBJECT_ARRAY;
  klass->value_validate = ligma_param_object_array_validate;
  klass->values_cmp     = ligma_param_object_array_values_cmp;
}

static void
ligma_param_object_array_init (GParamSpec *pspec)
{
}

static gboolean
ligma_param_object_array_validate (GParamSpec *pspec,
                                  GValue     *value)
{
  LigmaParamSpecObjectArray *array_spec = LIGMA_PARAM_SPEC_OBJECT_ARRAY (pspec);
  LigmaObjectArray          *array      = value->data[0].v_pointer;

  if (array)
    {
      gsize i;

      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }

      if (! g_type_is_a (array->object_type, array_spec->object_type))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }

      for (i = 0; i < array->length; i++)
        {
          if (array->data[i] && ! g_type_is_a (G_OBJECT_TYPE (array->data[i]),
                                               array_spec->object_type))
            {
              g_value_set_boxed (value, NULL);
              return TRUE;
            }
        }
    }

  return FALSE;
}

static gint
ligma_param_object_array_values_cmp (GParamSpec   *pspec,
                                    const GValue *value1,
                                    const GValue *value2)
{
  LigmaObjectArray *array1 = value1->data[0].v_pointer;
  LigmaObjectArray *array2 = value2->data[0].v_pointer;

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
 * ligma_param_spec_object_array:
 * @name:        Canonical name of the property specified.
 * @nick:        Nick name of the property specified.
 * @blurb:       Description of the property specified.
 * @object_type: GType of the array's elements.
 * @flags:       Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecObjectArray specifying a
 * #LIGMA_TYPE_OBJECT_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecObjectArray.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_object_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GType        object_type,
                              GParamFlags  flags)
{
  LigmaParamSpecObjectArray *array_spec;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);

  array_spec = g_param_spec_internal (LIGMA_TYPE_PARAM_OBJECT_ARRAY,
                                      name, nick, blurb, flags);

  g_return_val_if_fail (array_spec, NULL);

  array_spec->object_type = object_type;

  return G_PARAM_SPEC (array_spec);
}

/**
 * ligma_value_get_object_array:
 * @value: a #GValue holding a object #LigmaObjectArray.
 *
 * Returns: (transfer none): the internal array of objects.
 */
GObject **
ligma_value_get_object_array (const GValue *value)
{
  LigmaObjectArray *array;

  g_return_val_if_fail (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

/**
 * ligma_value_dup_object_array:
 * @value: a #GValue holding a object #LigmaObjectArray.
 *
 * Returns: (transfer full): a deep copy of the array of objects.
 */
GObject **
ligma_value_dup_object_array (const GValue *value)
{
  LigmaObjectArray *array;

  g_return_val_if_fail (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    {
      GObject **ret = g_memdup2 (array->data, (array->length) * sizeof (GObject *));
      gsize    i;

      for (i = 0; i < array->length; i++)
        g_object_ref (ret[i]);

      return ret;
    }

  return NULL;
}

/**
 * ligma_value_set_object_array:
 * @value: A valid value of type %LIGMA_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
ligma_value_set_object_array (GValue   *value,
                             GType     object_type,
                             GObject **data,
                             gsize     length)
{
  LigmaObjectArray *array;

  g_return_if_fail (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = ligma_object_array_new (object_type, data, length, FALSE);

  g_value_take_boxed (value, array);
}

/**
 * ligma_value_set_static_object_array:
 * @value: A valid value of type %LIGMA_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
ligma_value_set_static_object_array (GValue   *value,
                                    GType     object_type,
                                    GObject **data,
                                    gsize     length)
{
  LigmaObjectArray *array;

  g_return_if_fail (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = ligma_object_array_new (object_type, data, length, TRUE);

  g_value_take_boxed (value, array);
}

/**
 * ligma_value_take_object_array:
 * @value: A valid value of type %LIGMA_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (transfer full) (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
ligma_value_take_object_array (GValue   *value,
                              GType     object_type,
                              GObject **data,
                              gsize     length)
{
  LigmaObjectArray *array;

  g_return_if_fail (LIGMA_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = ligma_object_array_new (object_type, data, length, TRUE);
  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}
