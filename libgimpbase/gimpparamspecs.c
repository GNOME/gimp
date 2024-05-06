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
 * GIMP_TYPE_PARAM_CHOICE
 */

static void       gimp_param_choice_class_init        (GParamSpecClass *klass);
static void       gimp_param_choice_init              (GParamSpec      *pspec);
static void       gimp_param_choice_value_set_default (GParamSpec      *pspec,
                                                       GValue          *value);
static void       gimp_param_choice_finalize          (GParamSpec      *pspec);
static gboolean   gimp_param_choice_validate          (GParamSpec      *pspec,
                                                      GValue          *value);
static gint       gimp_param_choice_values_cmp        (GParamSpec      *pspec,
                                                      const GValue    *value1,
                                                      const GValue    *value2);

GType
gimp_param_choice_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_choice_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecChoice),
        0,
        (GInstanceInitFunc) gimp_param_choice_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamChoice", &info, 0);
    }

  return type;
}

static void
gimp_param_choice_class_init (GParamSpecClass *klass)
{
  klass->value_type        = G_TYPE_STRING;
  klass->value_set_default = gimp_param_choice_value_set_default;
  klass->finalize          = gimp_param_choice_finalize;
  klass->value_validate    = gimp_param_choice_validate;
  klass->values_cmp        = gimp_param_choice_values_cmp;
}

static void
gimp_param_choice_init (GParamSpec *pspec)
{
  GimpParamSpecChoice *choice = GIMP_PARAM_SPEC_CHOICE (pspec);

  choice->choice        = NULL;
  choice->default_value = NULL;
}

static void
gimp_param_choice_value_set_default (GParamSpec *pspec,
                                     GValue     *value)
{
  GimpParamSpecChoice *cspec = GIMP_PARAM_SPEC_CHOICE (pspec);

  g_value_set_string (value, cspec->default_value);
}

static void
gimp_param_choice_finalize (GParamSpec *pspec)
{
  GimpParamSpecChoice *spec_choice = GIMP_PARAM_SPEC_CHOICE (pspec);

  g_free (spec_choice->default_value);
  g_object_unref (spec_choice->choice);
}

static gboolean
gimp_param_choice_validate (GParamSpec *pspec,
                            GValue     *value)
{
  GimpParamSpecChoice *spec_choice = GIMP_PARAM_SPEC_CHOICE (pspec);
  GimpChoice          *choice      = spec_choice->choice;
  const gchar         *strval      = g_value_get_string (value);

  if (! gimp_choice_is_valid (choice, strval))
    {
      if (gimp_choice_is_valid (choice, spec_choice->default_value))
        {
          g_value_set_string (value, spec_choice->default_value);
        }
      else
        {
          /* This might happen if the default value is set insensitive. Then we
           * should just set any valid random nick.
           */
          GList *nicks;

          nicks = gimp_choice_list_nicks (choice);
          for (GList *iter = nicks; iter; iter = iter->next)
            if (gimp_choice_is_valid (choice, (gchar *) iter->data))
              {
                g_value_set_string (value, (gchar *) iter->data);
                break;
              }
        }
      return TRUE;
    }

  return FALSE;
}

static gint
gimp_param_choice_values_cmp (GParamSpec   *pspec,
                              const GValue *value1,
                              const GValue *value2)
{
  const gchar *choice1 = g_value_get_string (value1);
  const gchar *choice2 = g_value_get_string (value2);

  return g_strcmp0 (choice1, choice2);
}

/**
 * gimp_param_spec_choice:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @choice: (transfer full): the %GimpChoice describing allowed choices.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecChoice specifying a
 * #G_TYPE_STRING property.
 * This %GimpParamSpecChoice takes ownership of the reference on @choice.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecChoice.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_choice (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        GimpChoice  *choice,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  GimpParamSpecChoice *choice_spec;

  choice_spec = g_param_spec_internal (GIMP_TYPE_PARAM_CHOICE,
                                       name, nick, blurb, flags);

  g_return_val_if_fail (choice_spec, NULL);

  choice_spec->choice        = choice;
  choice_spec->default_value = g_strdup (default_value);

  return G_PARAM_SPEC (choice_spec);
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

/**
 * gimp_value_get_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 *
 * Gets the contents of a %GIMP_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer none) (array): The contents of @value
 */
const gint32 *
gimp_value_get_int32_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (const gint32 *) gimp_value_get_array (value);
}

/**
 * gimp_value_dup_int32_array:
 * @value: A valid value of type %GIMP_TYPE_INT32_ARRAY
 *
 * Gets the contents of a %GIMP_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer full) (array): The contents of @value
 */
gint32 *
gimp_value_dup_int32_array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

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
 * GIMP_TYPE_OBJECT_ARRAY
 */

/**
 * gimp_object_array_new:
 * @data:        (array length=length) (transfer none): an array of objects.
 * @object_type: the array will hold objects of this type
 * @length:      the length of @data.
 * @static_data: whether the objects in @data are static objects and don't
 *               need to be copied.
 *
 * Creates a new #GimpObjectArray containing object pointers, of size @length.
 *
 * If @static_data is %TRUE, @data is used as-is.
 *
 * If @static_data is %FALSE, the object and array will be re-allocated,
 * hence you are expected to free your input data after.
 *
 * Returns: (transfer full): a new #GimpObjectArray.
 */
GimpObjectArray *
gimp_object_array_new (GType     object_type,
                       GObject **data,
                       gsize     length,
                       gboolean  static_data)
{
  GimpObjectArray *array;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (GimpObjectArray);

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
 * gimp_object_array_copy:
 * @array: an original #GimpObjectArray of objects.
 *
 * Creates a new #GimpObjectArray containing a deep copy of @array.
 *
 * Returns: (transfer full): a new #GimpObjectArray.
 **/
GimpObjectArray *
gimp_object_array_copy (const GimpObjectArray *array)
{
  if (array)
    return gimp_object_array_new (array->object_type,
                                  array->data,
                                  array->length, FALSE);

  return NULL;
}

void
gimp_object_array_free (GimpObjectArray *array)
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

      g_slice_free (GimpObjectArray, array);
    }
}

G_DEFINE_BOXED_TYPE (GimpObjectArray, gimp_object_array, gimp_object_array_copy, gimp_object_array_free)


/*
 * GIMP_TYPE_PARAM_OBJECT_ARRAY
 */

static void       gimp_param_object_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_object_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_object_array_validate    (GParamSpec      *pspec,
                                                       GValue          *value);
static gint       gimp_param_object_array_values_cmp  (GParamSpec      *pspec,
                                                       const GValue    *value1,
                                                       const GValue    *value2);

GType
gimp_param_object_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_object_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecObjectArray),
        0,
        (GInstanceInitFunc) gimp_param_object_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamObjectArray", &info, 0);
    }

  return type;
}

static void
gimp_param_object_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_OBJECT_ARRAY;
  klass->value_validate = gimp_param_object_array_validate;
  klass->values_cmp     = gimp_param_object_array_values_cmp;
}

static void
gimp_param_object_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_object_array_validate (GParamSpec *pspec,
                                  GValue     *value)
{
  GimpParamSpecObjectArray *array_spec = GIMP_PARAM_SPEC_OBJECT_ARRAY (pspec);
  GimpObjectArray          *array      = value->data[0].v_pointer;

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
gimp_param_object_array_values_cmp (GParamSpec   *pspec,
                                    const GValue *value1,
                                    const GValue *value2)
{
  GimpObjectArray *array1 = value1->data[0].v_pointer;
  GimpObjectArray *array2 = value2->data[0].v_pointer;

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
 * gimp_param_spec_object_array:
 * @name:        Canonical name of the property specified.
 * @nick:        Nick name of the property specified.
 * @blurb:       Description of the property specified.
 * @object_type: GType of the array's elements.
 * @flags:       Flags for the property specified.
 *
 * Creates a new #GimpParamSpecObjectArray specifying a
 * [type@ObjectArray] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecObjectArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_object_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GType        object_type,
                              GParamFlags  flags)
{
  GimpParamSpecObjectArray *array_spec;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_OBJECT_ARRAY,
                                      name, nick, blurb, flags);

  g_return_val_if_fail (array_spec, NULL);

  array_spec->object_type = object_type;

  return G_PARAM_SPEC (array_spec);
}

/**
 * gimp_value_get_object_array:
 * @value: a #GValue holding a object #GimpObjectArray.
 *
 * Returns: (transfer none): the internal array of objects.
 */
GObject **
gimp_value_get_object_array (const GValue *value)
{
  GimpObjectArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

/**
 * gimp_value_dup_object_array:
 * @value: a #GValue holding a object #GimpObjectArray.
 *
 * Returns: (transfer full): a deep copy of the array of objects.
 */
GObject **
gimp_value_dup_object_array (const GValue *value)
{
  GimpObjectArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value), NULL);

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
 * gimp_value_set_object_array:
 * @value: A valid value of type %GIMP_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
gimp_value_set_object_array (GValue   *value,
                             GType     object_type,
                             GObject **data,
                             gsize     length)
{
  GimpObjectArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = gimp_object_array_new (object_type, data, length, FALSE);

  g_value_take_boxed (value, array);
}

/**
 * gimp_value_set_static_object_array:
 * @value: A valid value of type %GIMP_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
gimp_value_set_static_object_array (GValue   *value,
                                    GType     object_type,
                                    GObject **data,
                                    gsize     length)
{
  GimpObjectArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = gimp_object_array_new (object_type, data, length, TRUE);

  g_value_take_boxed (value, array);
}

/**
 * gimp_value_take_object_array:
 * @value: A valid value of type %GIMP_TYPE_OBJECT_ARRAY
 * @object_type: The #GType of the object elements
 * @data: (transfer full) (array length=length): A #GObject array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
gimp_value_take_object_array (GValue   *value,
                              GType     object_type,
                              GObject **data,
                              gsize     length)
{
  GimpObjectArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value));
  g_return_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT));

  array = gimp_object_array_new (object_type, data, length, TRUE);
  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}

/*
 * GIMP_TYPE_PARAM_EXPORT_OPTIONS
 */

static void       gimp_param_export_options_class_init        (GParamSpecClass *klass);
static void       gimp_param_export_options_init              (GParamSpec      *pspec);

GType
gimp_param_export_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_export_options_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecExportOptions),
        0,
        (GInstanceInitFunc) gimp_param_export_options_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamExportOptions", &info, 0);
    }

  return type;
}

static void
gimp_param_export_options_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_EXPORT_OPTIONS;
}

static void
gimp_param_export_options_init (GParamSpec *pspec)
{
  GimpParamSpecExportOptions *options = GIMP_PARAM_SPEC_EXPORT_OPTIONS (pspec);

  options->capabilities = 0;
}

/**
 * gimp_param_spec_export_options:
 * @name:         Canonical name of the property specified.
 * @nick:         Nick name of the property specified.
 * @blurb:        Description of the property specified.
 * @capabilities: Int representing the image export capabilities
 * @flags:        Flags for the property specified.
 *
 * Creates a new #GimpParamSpecExportOptions specifying a
 * #G_TYPE_INT property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecExportOptions.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_export_options (const gchar *name,
                                const gchar *nick,
                                const gchar *blurb,
                                gint         capabilities,
                                GParamFlags  flags)
{
  GimpParamSpecExportOptions *options_spec;

  options_spec = g_param_spec_internal (GIMP_TYPE_PARAM_EXPORT_OPTIONS,
                                        name, nick, blurb, flags);

  g_return_val_if_fail (options_spec, NULL);

  options_spec->capabilities = capabilities;

  return G_PARAM_SPEC (options_spec);
}
