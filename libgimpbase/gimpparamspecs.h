/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs.h
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_PARAM_SPECS_H__
#define __GIMP_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GIMP_PARAM_NO_VALIDATE:
 *
 * Since 3.0
 */
#define GIMP_PARAM_NO_VALIDATE    (1 << (0 + G_PARAM_USER_SHIFT))

/**
 * GIMP_PARAM_DONT_SERIALIZE:
 *
 * This property will be ignored when serializing and deserializing.
 * This is useful for GimpProcedure arguments for which you never want
 * the last run values to be restored.
 *
 * Since 3.0
 */
#define GIMP_PARAM_DONT_SERIALIZE (1 << (1 + G_PARAM_USER_SHIFT))

/**
 * GIMP_PARAM_FLAG_SHIFT:
 *
 * Minimum shift count to be used for libgimpconfig defined
 * [flags@GObject.ParamFlags] (see libgimpconfig/gimpconfig-params.h).
 */
#define GIMP_PARAM_FLAG_SHIFT     (2 + G_PARAM_USER_SHIFT)

/**
 * GIMP_PARAM_STATIC_STRINGS:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | \
                                   G_PARAM_STATIC_NICK | \
                                   G_PARAM_STATIC_BLURB)

/**
 * GIMP_PARAM_READABLE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_READABLE       (G_PARAM_READABLE    | \
                                   GIMP_PARAM_STATIC_STRINGS)

/**
 * GIMP_PARAM_WRITABLE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_WRITABLE       (G_PARAM_WRITABLE    | \
                                   GIMP_PARAM_STATIC_STRINGS)

/**
 * GIMP_PARAM_READWRITE:
 *
 * Since: 2.4
 **/
#define GIMP_PARAM_READWRITE      (G_PARAM_READWRITE   | \
                                   GIMP_PARAM_STATIC_STRINGS)


/*
 * GIMP_TYPE_PARAM_CHOICE
 */

#define GIMP_TYPE_PARAM_CHOICE           (gimp_param_choice_get_type ())
#define GIMP_PARAM_SPEC_CHOICE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CHOICE, GimpParamSpecChoice))
#define GIMP_IS_PARAM_SPEC_CHOICE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHOICE))

typedef struct _GimpParamSpecChoice GimpParamSpecChoice;

struct _GimpParamSpecChoice
{
  GParamSpecBoxed parent_instance;

  gchar          *default_value;
  GimpChoice     *choice;
};

GType        gimp_param_choice_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_choice     (const gchar  *name,
                                         const gchar  *nick,
                                         const gchar  *blurb,
                                         GimpChoice   *choice,
                                         const gchar  *default_value,
                                         GParamFlags   flags);


/*
 * GIMP_TYPE_ARRAY
 */

/**
 * GimpArray:
 * @data: (array length=length): pointer to the array's data.
 * @length:      length of @data, in bytes.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _GimpArray GimpArray;

struct _GimpArray
{
  guint8   *data;
  gsize     length;
  gboolean  static_data;
};

GimpArray * gimp_array_new  (const guint8    *data,
                             gsize            length,
                             gboolean         static_data);
GimpArray * gimp_array_copy (const GimpArray *array);
void        gimp_array_free (GimpArray       *array);

#define GIMP_TYPE_ARRAY               (gimp_array_get_type ())
#define GIMP_VALUE_HOLDS_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_ARRAY))

GType   gimp_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_ARRAY
 */

#define GIMP_TYPE_PARAM_ARRAY           (gimp_param_array_get_type ())
#define GIMP_PARAM_SPEC_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ARRAY, GimpParamSpecArray))
#define GIMP_IS_PARAM_SPEC_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ARRAY))

typedef struct _GimpParamSpecArray GimpParamSpecArray;

struct _GimpParamSpecArray
{
  GParamSpecBoxed parent_instance;
};

GType        gimp_param_array_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_array     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_INT32_ARRAY
 */

#define GIMP_TYPE_INT32_ARRAY               (gimp_int32_array_get_type ())
#define GIMP_VALUE_HOLDS_INT32_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT32_ARRAY))

GType   gimp_int32_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT32_ARRAY
 */

#define GIMP_TYPE_PARAM_INT32_ARRAY           (gimp_param_int32_array_get_type ())
#define GIMP_PARAM_SPEC_INT32_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT32_ARRAY, GimpParamSpecInt32Array))
#define GIMP_IS_PARAM_SPEC_INT32_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT32_ARRAY))

typedef struct _GimpParamSpecInt32Array GimpParamSpecInt32Array;

struct _GimpParamSpecInt32Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_int32_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int32_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gint32 * gimp_value_get_int32_array        (const GValue *value);
gint32       * gimp_value_dup_int32_array        (const GValue *value);
void           gimp_value_set_int32_array        (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           gimp_value_set_static_int32_array (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           gimp_value_take_int32_array       (GValue       *value,
                                                  gint32       *data,
                                                  gsize         length);


/*
 * GIMP_TYPE_FLOAT_ARRAY
 */

#define GIMP_TYPE_FLOAT_ARRAY               (gimp_float_array_get_type ())
#define GIMP_VALUE_HOLDS_FLOAT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_FLOAT_ARRAY))

GType   gimp_float_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_FLOAT_ARRAY
 */

#define GIMP_TYPE_PARAM_FLOAT_ARRAY           (gimp_param_float_array_get_type ())
#define GIMP_PARAM_SPEC_FLOAT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_FLOAT_ARRAY, GimpParamSpecFloatArray))
#define GIMP_IS_PARAM_SPEC_FLOAT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FLOAT_ARRAY))

typedef struct _GimpParamSpecFloatArray GimpParamSpecFloatArray;

struct _GimpParamSpecFloatArray
{
  GimpParamSpecArray parent_instance;
};

GType           gimp_param_float_array_get_type   (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_float_array       (const gchar  *name,
                                                   const gchar  *nick,
                                                   const gchar  *blurb,
                                                   GParamFlags   flags);

const gdouble * gimp_value_get_float_array        (const GValue  *value);
gdouble       * gimp_value_dup_float_array        (const GValue  *value);
void            gimp_value_set_float_array        (GValue        *value,
                                                   const gdouble *data,
                                                   gsize         length);
void            gimp_value_set_static_float_array (GValue        *value,
                                                   const gdouble *data,
                                                   gsize         length);
void            gimp_value_take_float_array       (GValue        *value,
                                                   gdouble       *data,
                                                   gsize         length);


/*
 * GIMP_TYPE_COLOR_ARRAY
 */

/**
 * GimpColorArray:
 *
 * A boxed type which is nothing more than an alias to a %NULL-terminated array
 * of [class@Gegl.Color].
 *
 * The code fragments in the following example show the use of a property of
 * type %GIMP_TYPE_COLOR_ARRAY with g_object_class_install_property(),
 * g_object_set() and g_object_get().
 *
 * ```C
 * g_object_class_install_property (object_class,
 *                                  PROP_COLORS,
 *                                  g_param_spec_boxed ("colors",
 *                                                      _("Colors"),
 *                                                      _("List of colors"),
 *                                                      GIMP_TYPE_COLOR_ARRAY,
 *                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
 *
 * GeglColor *colors[] = { gegl_color_new ("red"), gegl_color_new ("blue"), NULL };
 *
 * g_object_set (obj, "colors", colors, NULL);
 *
 * GeglColors **colors;
 *
 * g_object_get (obj, "colors", &colors, NULL);
 * gimp_color_array_free (colors);
 * ```
 *
 * Since: 3.0
 */
typedef GeglColor**                          GimpColorArray;

#define GIMP_TYPE_COLOR_ARRAY               (gimp_color_array_get_type ())
#define GIMP_VALUE_HOLDS_COLOR_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_COLOR_ARRAY))


GType            gimp_color_array_get_type            (void) G_GNUC_CONST;

GimpColorArray   gimp_color_array_copy                (GimpColorArray  array);
void             gimp_color_array_free                (GimpColorArray  array);
gint             gimp_color_array_get_length          (GimpColorArray  array);


/*
 * GIMP_TYPE_OBJECT_ARRAY
 */

/**
 * GimpObjectArray:
 * @object_type: #GType of the contained objects.
 * @data: (array length=length): pointer to the array's data.
 * @length:      length of @data, in number of objects.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _GimpObjectArray GimpObjectArray;

struct _GimpObjectArray
{
  GType     object_type;
  GObject **data;
  gsize     length;
  gboolean  static_data;
};

GimpObjectArray * gimp_object_array_new  (GType                  object_type,
                                          GObject              **data,
                                          gsize                  length,
                                          gboolean               static_data);
GimpObjectArray * gimp_object_array_copy (const GimpObjectArray  *array);
void              gimp_object_array_free (GimpObjectArray        *array);

#define GIMP_TYPE_OBJECT_ARRAY               (gimp_object_array_get_type ())
#define GIMP_VALUE_HOLDS_OBJECT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_OBJECT_ARRAY))

GType   gimp_object_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_OBJECT_ARRAY
 */

#define GIMP_TYPE_PARAM_OBJECT_ARRAY           (gimp_param_object_array_get_type ())
#define GIMP_PARAM_SPEC_OBJECT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_OBJECT_ARRAY, GimpParamSpecObjectArray))
#define GIMP_IS_PARAM_SPEC_OBJECT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_OBJECT_ARRAY))

typedef struct _GimpParamSpecObjectArray GimpParamSpecObjectArray;

struct _GimpParamSpecObjectArray
{
  GParamSpecBoxed parent_instance;

  GType           object_type;
};

GType         gimp_param_object_array_get_type   (void) G_GNUC_CONST;

GParamSpec  * gimp_param_spec_object_array       (const gchar   *name,
                                                  const gchar   *nick,
                                                  const gchar   *blurb,
                                                  GType          object_type,
                                                  GParamFlags    flags);

GObject    ** gimp_value_get_object_array        (const GValue  *value);
GObject    ** gimp_value_dup_object_array        (const GValue  *value);
void          gimp_value_set_object_array        (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);
void          gimp_value_set_static_object_array (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);
void          gimp_value_take_object_array       (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);


/*
 * GIMP_TYPE_PARAM_EXPORT_OPTIONS
 */

#define GIMP_TYPE_PARAM_EXPORT_OPTIONS           (gimp_param_export_options_get_type ())
#define GIMP_PARAM_SPEC_EXPORT_OPTIONS(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_EXPORT_OPTIONS, GimpParamSpecExportOptions))
#define GIMP_IS_PARAM_SPEC_EXPORT_OPTIONS(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_EXPORT_OPTIONS))

typedef struct _GimpParamSpecExportOptions GimpParamSpecExportOptions;

struct _GimpParamSpecExportOptions
{
  GParamSpecBoxed parent_instance;

  gint            capabilities;
};

GType        gimp_param_export_options_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_export_options     (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 gint          capabilities,
                                                 GParamFlags   flags);

G_END_DECLS

#endif  /*  __GIMP_PARAM_SPECS_H__  */
