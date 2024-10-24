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
 * GIMP_TYPE_PARAM_OBJECT
 */

static void         gimp_param_object_class_init            (GimpParamSpecObjectClass *klass);
static void         gimp_param_object_init                  (GimpParamSpecObject      *pspec);
static void         gimp_param_object_finalize              (GParamSpec               *pspec);
static void         gimp_param_object_value_set_default     (GParamSpec               *pspec,
                                                             GValue                   *value);

static GParamSpec * gimp_param_spec_object_real_duplicate   (GParamSpec               *pspec);
static GObject    * gimp_param_spec_object_real_get_default (GParamSpec               *pspec);


GType
gimp_param_object_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_object_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecObject),
        0,
        (GInstanceInitFunc) gimp_param_object_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamObject", &info, G_TYPE_FLAG_ABSTRACT);
    }

  return type;
}

static void
gimp_param_object_class_init (GimpParamSpecObjectClass *klass)
{
  GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (klass);

  klass->duplicate          = gimp_param_spec_object_real_duplicate;
  klass->get_default        = gimp_param_spec_object_real_get_default;

  pclass->value_type        = G_TYPE_OBJECT;
  pclass->finalize          = gimp_param_object_finalize;
  pclass->value_set_default = gimp_param_object_value_set_default;
}

static void
gimp_param_object_init (GimpParamSpecObject *ospec)
{
  ospec->_default_value = NULL;
}

static void
gimp_param_object_finalize (GParamSpec *pspec)
{
  GimpParamSpecObject *ospec        = GIMP_PARAM_SPEC_OBJECT (pspec);
  GParamSpecClass     *parent_class = g_type_class_peek (g_type_parent (GIMP_TYPE_PARAM_OBJECT));

  g_clear_object (&ospec->_default_value);

  parent_class->finalize (pspec);
}

static void
gimp_param_object_value_set_default (GParamSpec *pspec,
                                     GValue     *value)
{
  GObject *default_value;

  g_return_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec));

  default_value = gimp_param_spec_object_get_default (pspec);
  g_value_set_object (value, default_value);
}

static GParamSpec *
gimp_param_spec_object_real_duplicate (GParamSpec *pspec)
{
  GimpParamSpecObject *ospec;
  GimpParamSpecObject *duplicate;

  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  ospec = GIMP_PARAM_SPEC_OBJECT (pspec);
  duplicate = g_param_spec_internal (G_TYPE_FROM_INSTANCE (pspec),
                                     pspec->name,
                                     g_param_spec_get_nick (pspec),
                                     g_param_spec_get_blurb (pspec),
                                     pspec->flags);

  duplicate->_default_value = ospec->_default_value ? g_object_ref (ospec->_default_value) : NULL;
  duplicate->_has_default   = ospec->_has_default;

  return G_PARAM_SPEC (duplicate);
}

static GObject *
gimp_param_spec_object_real_get_default (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec), NULL);
  g_return_val_if_fail (GIMP_PARAM_SPEC_OBJECT (pspec)->_has_default, NULL);

  return GIMP_PARAM_SPEC_OBJECT (pspec)->_default_value;
}

/**
 * gimp_param_spec_object_get_default:
 * @pspec: a #GObject #GParamSpec
 *
 * Get the default object value of the param spec.
 *
 * If the @pspec has been registered with a specific default (which can
 * be verified with [func@Gimp.ParamSpecObject.has_default]), it will be
 * returned, though some specific subtypes may support returning dynamic
 * default (e.g. based on context).
 *
 * Returns: (transfer none): the default value.
 */
GObject *
gimp_param_spec_object_get_default (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  return GIMP_PARAM_SPEC_OBJECT_GET_CLASS (pspec)->get_default (pspec);
}

/**
 * gimp_param_spec_object_set_default:
 * @pspec: a #GObject #GParamSpec
 * @default_value: (transfer none) (nullable): a default value.
 *
 * Set the default object value of the param spec. This will switch the
 * `has_default` flag so that [func@Gimp.ParamSpecObject.has_default]
 * will now return %TRUE.
 *
 * A %NULL @default_value still counts as a default (unless the specific
 * @pspec does not allow %NULL as a default).
 */
void
gimp_param_spec_object_set_default (GParamSpec *pspec,
                                    GObject    *default_value)
{
  g_return_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec));

  GIMP_PARAM_SPEC_OBJECT (pspec)->_has_default = TRUE;
  g_set_object (&GIMP_PARAM_SPEC_OBJECT (pspec)->_default_value, default_value);
}

/**
 * gimp_param_spec_object_has_default:
 * @pspec: a #GObject #GParamSpec
 *
 * This function tells whether a default was set, typically with
 * [func@Gimp.ParamSpecObject.set_default] or any other way. It
 * does not guarantee that the default is an actual object (it may be
 * %NULL if valid as a default).
 *
 * Returns: whether a default value was set.
 */
gboolean
gimp_param_spec_object_has_default (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec), FALSE);

  return GIMP_PARAM_SPEC_OBJECT (pspec)->_has_default;
}

/**
 * gimp_param_spec_object_duplicate:
 * @pspec: a [struct@Gimp.ParamSpecObject].
 *
 * This function duplicates @pspec appropriately, depending on the
 * accurate spec type.
 *
 * Returns: (transfer floating): a newly created param spec.
 */
GParamSpec *
gimp_param_spec_object_duplicate (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  return GIMP_PARAM_SPEC_OBJECT_GET_CLASS (pspec)->duplicate (pspec);
}


/*
 * GIMP_TYPE_ARRAY
 */

/**
 * gimp_array_new:
 * @data: (array length=length):
 * @length:
 * @static_data:
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

  array->data        = static_data ? (guint8 *) data : g_memdup2 (data, length);
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

G_DEFINE_BOXED_TYPE (GimpArray, gimp_array, gimp_array_copy, gimp_array_free)


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
 * [type@Array] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecArray.
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
    return g_memdup2 (array->data, array->length);

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
 * GIMP_TYPE_INT32_ARRAY
 */

typedef GimpArray GimpInt32Array;
G_DEFINE_BOXED_TYPE (GimpInt32Array, gimp_int32_array, gimp_array_copy, gimp_array_free)

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
        sizeof (GimpParamSpecInt32Array),
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
 * %GIMP_TYPE_INT32_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecInt32Array.
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

/**
 * gimp_value_get_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 * @length: the number of returned #int32 elements.
 *
 * Gets the contents of a %GIMP_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer none) (array length=length): The contents of @value
 */
const gint32 *
gimp_value_get_int32_array (const GValue *value,
                            gsize        *length)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (const gint32 *) gimp_value_get_array (value);
}

/**
 * gimp_value_dup_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 * @length: the number of returned #int32 elements.
 *
 * Gets the contents of a %GIMP_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer full) (array length=length): The contents of @value
 */
gint32 *
gimp_value_dup_int32_array (const GValue *value,
                            gsize        *length)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (gint32 *) gimp_value_dup_array (value);
}

/**
 * gimp_value_set_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
gimp_value_set_int32_array (GValue       *value,
                            const gint32 *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint32));
}

/**
 * gimp_value_set_static_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
gimp_value_set_static_int32_array (GValue       *value,
                                   const gint32 *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint32));
}

/**
 * gimp_value_take_int32_array:
 * @value: A valid value of type %GIMP_TYPE_int32_ARRAY
 * @data: (transfer full) (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
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

typedef GimpArray GimpFloatArray;
G_DEFINE_BOXED_TYPE (GimpFloatArray, gimp_float_array, gimp_array_copy, gimp_array_free)

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
        sizeof (GimpParamSpecFloatArray),
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
 * %GIMP_TYPE_FLOAT_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecFloatArray.
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

/**
 * gimp_value_get_float_array:
 * @value: A valid value of type %GIMP_TYPE_FLOAT_ARRAY
 *
 * Gets the contents of a %GIMP_TYPE_FLOAT_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const gdouble *
gimp_value_get_float_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (const gdouble *) gimp_value_get_array (value);
}

/**
 * gimp_value_dup_float_array:
 * @value: A valid value of type %GIMP_TYPE_FLOAT_ARRAY
 *
 * Gets the contents of a %GIMP_TYPE_FLOAT_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
gdouble *
gimp_value_dup_float_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (gdouble *) gimp_value_dup_array (value);
}

/**
 * gimp_value_set_float_array:
 * @value: A valid value of type %GIMP_TYPE_FLOAT_ARRAY
 * @data: (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
gimp_value_set_float_array (GValue        *value,
                            const gdouble *data,
                            gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

/**
 * gimp_value_set_static_float_array:
 * @value: A valid value of type %GIMP_TYPE_FLOAT_ARRAY
 * @data: (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
gimp_value_set_static_float_array (GValue        *value,
                                   const gdouble *data,
                                   gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

/**
 * gimp_value_take_float_array:
 * @value: A valid value of type %GIMP_TYPE_FLOAT_ARRAY
 * @data: (transfer full) (array length=length): A #gfloat array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
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
 * GIMP_TYPE_COLOR_ARRAY
 */

GType
gimp_color_array_get_type (void)
{
  static gsize static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      GType g_define_type_id =
        g_boxed_type_register_static (g_intern_static_string ("GimpColorArray"),
                                      (GBoxedCopyFunc) gimp_color_array_copy,
                                      (GBoxedFreeFunc) gimp_color_array_free);

      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

/**
 * gimp_color_array_copy:
 * @array: an array of colors.
 *
 * Creates a new #GimpColorArray containing a deep copy of a %NULL-terminated
 * array of [class@Gegl.Color].
 *
 * Returns: (transfer full): a new #GimpColorArray.
 **/
GimpColorArray
gimp_color_array_copy (GimpColorArray array)
{
  GeglColor **copy;
  gint        length = gimp_color_array_get_length (array);

  copy = g_malloc0 (sizeof (GeglColor *) * (length + 1));

  for (gint i = 0; i < length; i++)
    copy[i] = gegl_color_duplicate (array[i]);

  return copy;
}

/**
 * gimp_color_array_free:
 * @array: an array of colors.
 *
 * Frees a %NULL-terminated array of [class@Gegl.Color].
 **/
void
gimp_color_array_free (GimpColorArray array)
{
  gint i = 0;

  while (array[i] != NULL)
    g_object_unref (array[i++]);

  g_free (array);
}

/**
 * gimp_color_array_get_length:
 * @array: an array of colors.
 *
 * Returns: the number of [class@Gegl.Color] in @array.
 **/
gint
gimp_color_array_get_length (GimpColorArray array)
{
  gint length = 0;

  while (array[length] != NULL)
    length++;

  return length;
}


/*
 * GIMP_TYPE_CORE_OBJECT_ARRAY
 */

static GimpCoreObjectArray gimp_core_object_array_copy (GimpCoreObjectArray array);

GType
gimp_core_object_array_get_type (void)
{
  static gsize static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      GType g_define_type_id =
        g_boxed_type_register_static (g_intern_static_string ("GimpCoreObjectArray"),
                                      (GBoxedCopyFunc) gimp_core_object_array_copy,
                                      (GBoxedFreeFunc) g_free);

      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

/**
 * gimp_core_object_array_get_length:
 * @array: a %NULL-terminated array of objects.
 *
 * Returns: the number of [class@GObject.Object] in @array.
 **/
gsize
gimp_core_object_array_get_length (GObject **array)
{
  gsize length = 0;

  while (array && array[length] != NULL)
    length++;

  return length;
}

/**
 * gimp_core_object_array_copy:
 * @array: an array of core_objects.
 *
 * Duplicate a new #GimpCoreObjectArray, which is basically simply a
 * %NULL-terminated array of [class@GObject.Object]. Note that you
 * should only use this alias type for arrays of core type objects
 * internally hold by `libgimp`, such as layers, channels, paths, images
 * and so on, because no reference is hold to the element objects.
 *
 * As a consequence, the copy also makes a shallow copy of the elements.
 *
 * Returns: (transfer container) (array zero-terminated=1): a new #GimpCoreObjectArray.
 **/
static GimpCoreObjectArray
gimp_core_object_array_copy (GimpCoreObjectArray array)
{
  GObject **copy;
  gsize     length = gimp_core_object_array_get_length (array);

  copy = g_malloc0 (sizeof (GObject *) * (length + 1));

  for (gint i = 0; i < length; i++)
    copy[i] = array[i];

  return copy;
}


/*
 * GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY
 */

static void       gimp_param_core_object_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_core_object_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_core_object_array_validate    (GParamSpec      *pspec,
                                                            GValue          *value);
static gint       gimp_param_core_object_array_values_cmp  (GParamSpec      *pspec,
                                                            const GValue    *value1,
                                                            const GValue    *value2);

GType
gimp_param_core_object_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_core_object_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecCoreObjectArray),
        0,
        (GInstanceInitFunc) gimp_param_core_object_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamCoreObjectArray", &info, 0);
    }

  return type;
}

static void
gimp_param_core_object_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_CORE_OBJECT_ARRAY;
  klass->value_validate = gimp_param_core_object_array_validate;
  klass->values_cmp     = gimp_param_core_object_array_values_cmp;
}

static void
gimp_param_core_object_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_core_object_array_validate (GParamSpec *pspec,
                                       GValue     *value)
{
  GimpParamSpecCoreObjectArray  *array_spec = GIMP_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec);
  GObject                      **array      = value->data[0].v_pointer;

  if (array)
    {
      gsize length = gimp_core_object_array_get_length (array);
      gsize i;

      if (length == 0)
        {
          g_value_set_boxed (value, NULL);
          return FALSE;
        }

      for (i = 0; i < length; i++)
        {
          if (array[i] && ! g_type_is_a (G_OBJECT_TYPE (array[i]), array_spec->object_type))
            {
              g_value_set_boxed (value, NULL);
              return TRUE;
            }
        }
    }

  return FALSE;
}

static gint
gimp_param_core_object_array_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2)
{
  GObject **array1 = value1->data[0].v_pointer;
  GObject **array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (gimp_core_object_array_get_length (array1) < gimp_core_object_array_get_length (array2))
    return -1;
  else if (gimp_core_object_array_get_length (array1) > gimp_core_object_array_get_length (array2))
    return 1;

  for (gsize i = 0; i < gimp_core_object_array_get_length (array1); i++)
    if (array1[0] != array2[0])
      return 1;

  return 0;
}

/**
 * gimp_param_spec_core_object_array:
 * @name:        Canonical name of the property specified.
 * @nick:        Nick name of the property specified.
 * @blurb:       Description of the property specified.
 * @object_type: GType of the array's elements.
 * @flags:       Flags for the property specified.
 *
 * Creates a new #GimpParamSpecCoreObjectArray specifying a
 * [type@CoreObjectArray] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecCoreObjectArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_core_object_array (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   GType        object_type,
                                   GParamFlags  flags)
{
  GimpParamSpecCoreObjectArray *array_spec;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY,
                                      name, nick, blurb, flags);

  g_return_val_if_fail (array_spec, NULL);

  array_spec->object_type = object_type;

  return G_PARAM_SPEC (array_spec);
}
