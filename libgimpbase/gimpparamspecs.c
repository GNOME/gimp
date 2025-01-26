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
 * GIMP_TYPE_PARAM_FILE
 */

#define GIMP_PARAM_SPEC_FILE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_FILE, GimpParamSpecFile))

typedef struct _GimpParamSpecFile GimpParamSpecFile;

struct _GimpParamSpecFile
{
  GimpParamSpecObject   parent_instance;

  /*< private >*/
  GimpFileChooserAction action;
  gboolean              none_ok;
};

static void         gimp_param_file_class_init     (GimpParamSpecObjectClass *klass);
static void         gimp_param_file_init           (GimpParamSpecFile        *fspec);

static gboolean     gimp_param_spec_file_validate  (GParamSpec               *pspec,
                                                    GValue                   *value);
static gint         gimp_param_spec_file_cmp       (GParamSpec               *pspec,
                                                    const GValue             *value1,
                                                    const GValue             *value2);
static GParamSpec * gimp_param_spec_file_duplicate (GParamSpec               *pspec);


/**
 * gimp_param_file_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a file param object.
 *
 * Since: 3.0
 **/
GType
gimp_param_file_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_file_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecFile),
        0,
        (GInstanceInitFunc) gimp_param_file_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_OBJECT,
                                     "GimpParamFile", &info, 0);
    }

  return type;
}

static void
gimp_param_file_class_init (GimpParamSpecObjectClass *klass)
{
  GParamSpecClass          *pclass = G_PARAM_SPEC_CLASS (klass);
  GimpParamSpecObjectClass *oclass = GIMP_PARAM_SPEC_OBJECT_CLASS (klass);

  pclass->value_type     = G_TYPE_FILE;
  pclass->value_validate = gimp_param_spec_file_validate;
  pclass->values_cmp     = gimp_param_spec_file_cmp;
  oclass->duplicate      = gimp_param_spec_file_duplicate;
}

static void
gimp_param_file_init (GimpParamSpecFile *fspec)
{
  fspec->none_ok = TRUE;
  fspec->action  = GIMP_FILE_CHOOSER_ACTION_OPEN;
}

static gboolean
gimp_param_spec_file_validate (GParamSpec *pspec,
                               GValue     *value)
{
  GimpParamSpecFile   *fspec     = GIMP_PARAM_SPEC_FILE (pspec);
  GimpParamSpecObject *ospec     = GIMP_PARAM_SPEC_OBJECT (pspec);
  GFile               *file      = value->data[0].v_pointer;
  gboolean             modifying = FALSE;

  if (file == NULL && ! fspec->none_ok && ospec->_default_value != NULL)
    {
      modifying = TRUE;
    }
  else if (file != NULL && g_file_is_native (file))
    {
      gboolean  exists = g_file_query_exists (file, NULL);
      GFileType type   = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      switch (fspec->action)
        {
        case GIMP_FILE_CHOOSER_ACTION_OPEN:
          modifying = (! exists || type != G_FILE_TYPE_REGULAR);
          break;
        case GIMP_FILE_CHOOSER_ACTION_SAVE:
          modifying = (exists && type != G_FILE_TYPE_REGULAR);
          break;
        case GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER:
          modifying = (! exists || type != G_FILE_TYPE_DIRECTORY);
          break;
        case GIMP_FILE_CHOOSER_ACTION_CREATE_FOLDER:
          modifying = (exists && type != G_FILE_TYPE_DIRECTORY);
          break;
        case GIMP_FILE_CHOOSER_ACTION_ANY:
          break;
        }
    }

  if (modifying)
    {
      g_clear_object (&file);
      value->data[0].v_pointer = ospec->_default_value ? g_object_ref (ospec->_default_value) : NULL;
    }

  return modifying;
}

static gint
gimp_param_spec_file_cmp (GParamSpec   *pspec,
                          const GValue *value1,
                          const GValue *value2)
{
  GFile *file1 = g_value_get_object (value1);
  GFile *file2 = g_value_get_object (value2);
  gchar *uri1;
  gchar *uri2;
  gint   retval;

  if (! file1 || ! file2)
    return file2 ? -1 : (file1 ? 1 : 0);

  uri1 = g_file_get_uri (file1);
  uri2 = g_file_get_uri (file2);

  retval = g_strcmp0 (uri1, uri2);

  g_free (uri1);
  g_free (uri2);

  return retval;
}

static GParamSpec *
gimp_param_spec_file_duplicate (GParamSpec *pspec)
{
  GimpParamSpecObject *ospec;
  GimpParamSpecFile   *fspec;
  GParamSpec          *duplicate;

  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_FILE (pspec), NULL);

  ospec     = GIMP_PARAM_SPEC_OBJECT (pspec);
  fspec     = GIMP_PARAM_SPEC_FILE (pspec);
  duplicate = gimp_param_spec_file (pspec->name,
                                    g_param_spec_get_nick (pspec),
                                    g_param_spec_get_blurb (pspec),
                                    fspec->action, fspec->none_ok,
                                    G_FILE (ospec->_default_value),
                                    pspec->flags);
  return duplicate;
}

/**
 * gimp_param_spec_file:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @action:        The type of file to expect.
 * @none_ok:       Whether %NULL is allowed.
 * @default_value: (nullable): File to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a file param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_file (const gchar           *name,
                      const gchar           *nick,
                      const gchar           *blurb,
                      GimpFileChooserAction  action,
                      gboolean               none_ok,
                      GFile                 *default_value,
                      GParamFlags            flags)
{
  GimpParamSpecFile   *fspec;
  GimpParamSpecObject *ospec;

  g_return_val_if_fail (default_value == NULL || G_IS_FILE (default_value), NULL);

  fspec = g_param_spec_internal (GIMP_TYPE_PARAM_FILE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (fspec, NULL);

  fspec->action         = action;
  fspec->none_ok        = none_ok;

  ospec                 = GIMP_PARAM_SPEC_OBJECT (fspec);
  ospec->_has_default   = TRUE;
  /* Note that we don't check none_ok and allows even NULL as default
   * value. What we won't allow will be trying to set a NULL value
   * later.
   */
  ospec->_default_value = default_value ? g_object_ref (G_OBJECT (default_value)) : NULL;

  return G_PARAM_SPEC (fspec);
}

/**
 * gimp_param_spec_file_get_action:
 * @pspec: a #GParamSpec to hold a #GFile value.
 *
 * Returns: the file action tied to @pspec.
 *
 * Since: 3.0
 **/
GimpFileChooserAction
gimp_param_spec_file_get_action (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_FILE (pspec), GIMP_FILE_CHOOSER_ACTION_ANY);

  return GIMP_PARAM_SPEC_FILE (pspec)->action;
}

/**
 * gimp_param_spec_file_set_action:
 * @pspec:  a #GParamSpec to hold a #GFile value.
 * @action: new action for @pspec.
 *
 * Change the file action tied to @pspec.
 *
 * Since: 3.0
 **/
void
gimp_param_spec_file_set_action (GParamSpec            *pspec,
                                 GimpFileChooserAction  action)
{
  g_return_if_fail (GIMP_IS_PARAM_SPEC_FILE (pspec));

  GIMP_PARAM_SPEC_FILE (pspec)->action = action;
}

/**
 * gimp_param_spec_file_none_allowed:
 * @pspec: a #GParamSpec to hold a #GFile value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
gimp_param_spec_file_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_FILE (pspec), FALSE);

  return GIMP_PARAM_SPEC_FILE (pspec)->none_ok;
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
        sizeof (GParamSpecBoxed),
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
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
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

/**
 * gimp_int32_array_get_values:
 * @array: the #GimpArray representing #int32 values.
 * @length: the number of #int32 values in the returned array.
 *
 * Returns: (array length=length) (transfer none): a C-array of #gint32.
 */
const gint32 *
gimp_int32_array_get_values (GimpArray *array,
                             gsize     *length)
{
  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (const gint32 *) array->data;
}

/**
 * gimp_int32_array_set_values:
 * @array: the array to modify.
 * @values: (array length=length): the C-array.
 * @length: the number of #int32 values in @data.
 * @static_data: whether @data is a static rather than allocated array.
 */
void
gimp_int32_array_set_values (GimpArray    *array,
                             const gint32 *values,
                             gsize         length,
                             gboolean      static_data)
{
  g_return_if_fail ((values == NULL && length == 0) || (values != NULL && length  > 0));

  if (! array->static_data)
    g_free (array->data);

  array->length      = length * sizeof (gint32);
  array->data        = static_data ? (guint8 *) values : g_memdup2 (values, array->length);
  array->static_data = static_data;
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
        sizeof (GParamSpecBoxed),
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
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT32_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
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
 * GIMP_TYPE_DOUBLE_ARRAY
 */

typedef GimpArray GimpDoubleArray;
G_DEFINE_BOXED_TYPE (GimpDoubleArray, gimp_double_array, gimp_array_copy, gimp_array_free)

/**
 * gimp_double_array_get_values:
 * @array: the #GimpArray representing #double values.
 * @length: the number of #double values in the returned array.
 *
 * Returns: (array length=length) (transfer none): a C-array of #gdouble.
 */
const gdouble *
gimp_double_array_get_values (GimpArray *array,
                              gsize     *length)
{
  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (const gdouble *) array->data;
}

/**
 * gimp_double_array_set_values:
 * @array: the array to modify.
 * @values: (array length=length): the C-array.
 * @length: the number of #double values in @data.
 * @static_data: whether @data is a static rather than allocated array.
 */
void
gimp_double_array_set_values (GimpArray     *array,
                              const gdouble *values,
                              gsize          length,
                              gboolean       static_data)
{
  g_return_if_fail ((values == NULL && length == 0) || (values != NULL && length  > 0));

  if (! array->static_data)
    g_free (array->data);

  array->length      = length * sizeof (gdouble);
  array->data        = static_data ? (guint8 *) values : g_memdup2 (values, array->length);
  array->static_data = static_data;
}


/*
 * GIMP_TYPE_PARAM_DOUBLE_ARRAY
 */

static void   gimp_param_double_array_class_init (GParamSpecClass *klass);
static void   gimp_param_double_array_init       (GParamSpec      *pspec);

GType
gimp_param_double_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_double_array_class_init,
        NULL, NULL,
        sizeof (GParamSpecBoxed),
        0,
        (GInstanceInitFunc) gimp_param_double_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamDoubleArray", &info, 0);
    }

  return type;
}

static void
gimp_param_double_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_DOUBLE_ARRAY;
}

static void
gimp_param_double_array_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_double_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #GimpParamSpecDoubleArray specifying a
 * %GIMP_TYPE_DOUBLE_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #GimpParamSpecDoubleArray.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_double_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GParamFlags  flags)
{
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_DOUBLE_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
}

/**
 * gimp_value_get_double_array:
 * @value: A valid value of type %GIMP_TYPE_DOUBLE_ARRAY
 * @length: the number of returned #double elements.
 *
 * Gets the contents of a %GIMP_TYPE_DOUBLE_ARRAY #GValue
 *
 * Returns: (transfer none) (array length=length): The contents of @value
 */
const gdouble *
gimp_value_get_double_array (const GValue *value,
                             gsize        *length)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (const gdouble *) gimp_value_get_array (value);
}

/**
 * gimp_value_dup_double_array:
 * @value: A valid value of type %GIMP_TYPE_DOUBLE_ARRAY
 * @length: the number of returned #double elements.
 *
 * Gets the contents of a %GIMP_TYPE_DOUBLE_ARRAY #GValue
 *
 * Returns: (transfer full) (array length=length): The contents of @value
 */
gdouble *
gimp_value_dup_double_array (const GValue *value,
                             gsize        *length)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (gdouble *) gimp_value_dup_array (value);
}

/**
 * gimp_value_set_double_array:
 * @value: A valid value of type %GIMP_TYPE_DOUBLE_ARRAY
 * @data: (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
gimp_value_set_double_array (GValue        *value,
                             const gdouble *data,
                             gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

/**
 * gimp_value_set_static_double_array:
 * @value: A valid value of type %GIMP_TYPE_DOUBLE_ARRAY
 * @data: (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
gimp_value_set_static_double_array (GValue        *value,
                                    const gdouble *data,
                                    gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

/**
 * gimp_value_take_double_array:
 * @value: A valid value of type %GIMP_TYPE_DOUBLE_ARRAY
 * @data: (transfer full) (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
gimp_value_take_double_array (GValue  *value,
                              gdouble *data,
                              gsize    length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value));

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

#define GIMP_PARAM_SPEC_CORE_OBJECT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY, GimpParamSpecCoreObjectArray))

typedef struct _GimpParamSpecCoreObjectArray GimpParamSpecCoreObjectArray;

struct _GimpParamSpecCoreObjectArray
{
  GParamSpecBoxed parent_instance;

  GType           object_type;
};

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

/**
 * gimp_param_spec_core_object_array_get_object_type:
 * @pspec: a #GParamSpec to hold a #GimpParamSpecCoreObjectArray value.
 *
 * Returns: the type for objects in the object array.
 *
 * Since: 3.0
 **/
GType
gimp_param_spec_core_object_array_get_object_type (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec), G_TYPE_NONE);

  return GIMP_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec)->object_type;
}
