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
/*
 * Keep in sync with libgimpconfig/gimpconfig-params.h
 */
#define GIMP_PARAM_NO_VALIDATE (1 << (6 + G_PARAM_USER_SHIFT))

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
 * GIMP_TYPE_ARRAY
 */

/**
 * GimpArray:
 * @data:        pointer to the array's data.
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
 * GIMP_TYPE_UINT8_ARRAY
 */

#define GIMP_TYPE_UINT8_ARRAY               (gimp_uint8_array_get_type ())
#define GIMP_VALUE_HOLDS_UINT8_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_UINT8_ARRAY))

GType   gimp_uint8_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_UINT8_ARRAY
 */

#define GIMP_TYPE_PARAM_UINT8_ARRAY           (gimp_param_uint8_array_get_type ())
#define GIMP_PARAM_SPEC_UINT8_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UINT8_ARRAY, GimpParamSpecUInt8Array))
#define GIMP_IS_PARAM_SPEC_UINT8_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_UINT8_ARRAY))

typedef struct _GimpParamSpecUInt8Array GimpParamSpecUInt8Array;

struct _GimpParamSpecUInt8Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_uint8_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_uint8_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const guint8 * gimp_value_get_uint8_array        (const GValue *value);
guint8       * gimp_value_dup_uint8_array        (const GValue *value);
void           gimp_value_set_uint8_array        (GValue       *value,
                                                  const guint8 *array,
                                                  gsize         length);
void           gimp_value_set_static_uint8_array (GValue       *value,
                                                  const guint8 *array,
                                                  gsize         length);
void           gimp_value_take_uint8_array       (GValue       *value,
                                                  guint8       *array,
                                                  gsize         length);


/*
 * GIMP_TYPE_INT16_ARRAY
 */

#define GIMP_TYPE_INT16_ARRAY               (gimp_int16_array_get_type ())
#define GIMP_VALUE_HOLDS_INT16_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT16_ARRAY))

GType   gimp_int16_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT16_ARRAY
 */

#define GIMP_TYPE_PARAM_INT16_ARRAY           (gimp_param_int16_array_get_type ())
#define GIMP_PARAM_SPEC_INT16_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT16_ARRAY, GimpParamSpecInt16Array))
#define GIMP_IS_PARAM_SPEC_INT16_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT16_ARRAY))

typedef struct _GimpParamSpecInt16Array GimpParamSpecInt16Array;

struct _GimpParamSpecInt16Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_int16_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int16_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gint16 * gimp_value_get_int16_array        (const GValue *value);
gint16       * gimp_value_dup_int16_array        (const GValue *value);
void           gimp_value_set_int16_array        (GValue       *value,
                                                  const gint16 *array,
                                                  gsize         length);
void           gimp_value_set_static_int16_array (GValue       *value,
                                                  const gint16 *array,
                                                  gsize         length);
void           gimp_value_take_int16_array       (GValue       *value,
                                                  gint16       *array,
                                                  gsize         length);


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
                                                  const gint32 *array,
                                                  gsize         length);
void           gimp_value_set_static_int32_array (GValue       *value,
                                                  const gint32 *array,
                                                  gsize         length);
void           gimp_value_take_int32_array       (GValue       *value,
                                                  gint32       *array,
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
                                                   const gdouble *array,
                                                   gsize         length);
void            gimp_value_set_static_float_array (GValue        *value,
                                                   const gdouble *array,
                                                   gsize         length);
void            gimp_value_take_float_array       (GValue        *value,
                                                   gdouble       *array,
                                                   gsize         length);


/*
 * GIMP_TYPE_STRING_ARRAY
 */

/**
 * GimpStringArray:
 * @data:        pointer to the array's data.
 * @length:      length of @data, in number of strings.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _GimpStringArray GimpStringArray;

struct _GimpStringArray
{
  gchar   **data;
  gsize     length;
  gboolean  static_data;
};

GimpStringArray * gimp_string_array_new  (const gchar          **data,
                                          gsize                  length,
                                          gboolean               static_data);
GimpStringArray * gimp_string_array_copy (const GimpStringArray  *array);
void              gimp_string_array_free (GimpStringArray        *array);

#define GIMP_TYPE_STRING_ARRAY               (gimp_string_array_get_type ())
#define GIMP_VALUE_HOLDS_STRING_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_STRING_ARRAY))

GType   gimp_string_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_STRING_ARRAY
 */

#define GIMP_TYPE_PARAM_STRING_ARRAY           (gimp_param_string_array_get_type ())
#define GIMP_PARAM_SPEC_STRING_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_STRING_ARRAY, GimpParamSpecStringArray))
#define GIMP_IS_PARAM_SPEC_STRING_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_STRING_ARRAY))

typedef struct _GimpParamSpecStringArray GimpParamSpecStringArray;

struct _GimpParamSpecStringArray
{
  GParamSpecBoxed parent_instance;
};

GType          gimp_param_string_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_string_array       (const gchar  *name,
                                                   const gchar  *nick,
                                                   const gchar  *blurb,
                                                   GParamFlags   flags);

const gchar ** gimp_value_get_string_array        (const GValue *value);
gchar       ** gimp_value_dup_string_array        (const GValue *value);
void           gimp_value_set_string_array        (GValue       *value,
                                                   const gchar **array,
                                                   gsize         length);
void           gimp_value_set_static_string_array (GValue       *value,
                                                   const gchar **array,
                                                   gsize         length);
void           gimp_value_take_string_array       (GValue       *value,
                                                   gchar       **array,
                                                   gsize         length);


/*
 * GIMP_TYPE_RGB_ARRAY
 */

#define GIMP_TYPE_RGB_ARRAY               (gimp_rgb_array_get_type ())
#define GIMP_VALUE_HOLDS_RGB_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_RGB_ARRAY))

GType   gimp_rgb_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_RGB_ARRAY
 */

#define GIMP_TYPE_PARAM_RGB_ARRAY           (gimp_param_rgb_array_get_type ())
#define GIMP_PARAM_SPEC_RGB_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_RGB_ARRAY, GimpParamSpecRGBArray))
#define GIMP_IS_PARAM_SPEC_RGB_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RGB_ARRAY))

typedef struct _GimpParamSpecRGBArray GimpParamSpecRGBArray;

struct _GimpParamSpecRGBArray
{
  GParamSpecBoxed parent_instance;
};

GType           gimp_param_rgb_array_get_type   (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_rgb_array       (const gchar   *name,
                                                 const gchar   *nick,
                                                 const gchar   *blurb,
                                                 GParamFlags    flags);

const GimpRGB * gimp_value_get_rgb_array        (const GValue  *value);
GimpRGB       * gimp_value_dup_rgb_array        (const GValue  *value);
void            gimp_value_set_rgb_array        (GValue        *value,
                                                 const GimpRGB *array,
                                                 gsize          length);
void            gimp_value_set_static_rgb_array (GValue        *value,
                                                 const GimpRGB *array,
                                                 gsize          length);
void            gimp_value_take_rgb_array       (GValue        *value,
                                                 GimpRGB       *array,
                                                 gsize          length);


/*
 * GIMP_TYPE_OBJECT_ARRAY
 */

/**
 * GimpObjectArray:
 * @object_type: #GType of the contained objects.
 * @data:        pointer to the array's data.
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
                                                  GObject      **array,
                                                  gsize          length);
void          gimp_value_set_static_object_array (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **array,
                                                  gsize          length);
void          gimp_value_take_object_array       (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **array,
                                                  gsize          length);


G_END_DECLS

#endif  /*  __GIMP_PARAM_SPECS_H__  */
