/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaparamspecs.h
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_PARAM_SPECS_H__
#define __LIGMA_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LIGMA_PARAM_NO_VALIDATE:
 *
 * Since 3.0
 */
/*
 * Keep in sync with libligmaconfig/ligmaconfig-params.h
 */
#define LIGMA_PARAM_NO_VALIDATE (1 << (6 + G_PARAM_USER_SHIFT))

/**
 * LIGMA_PARAM_STATIC_STRINGS:
 *
 * Since: 2.4
 **/
#define LIGMA_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | \
                                   G_PARAM_STATIC_NICK | \
                                   G_PARAM_STATIC_BLURB)

/**
 * LIGMA_PARAM_READABLE:
 *
 * Since: 2.4
 **/
#define LIGMA_PARAM_READABLE       (G_PARAM_READABLE    | \
                                   LIGMA_PARAM_STATIC_STRINGS)

/**
 * LIGMA_PARAM_WRITABLE:
 *
 * Since: 2.4
 **/
#define LIGMA_PARAM_WRITABLE       (G_PARAM_WRITABLE    | \
                                   LIGMA_PARAM_STATIC_STRINGS)

/**
 * LIGMA_PARAM_READWRITE:
 *
 * Since: 2.4
 **/
#define LIGMA_PARAM_READWRITE      (G_PARAM_READWRITE   | \
                                   LIGMA_PARAM_STATIC_STRINGS)


/*
 * LIGMA_TYPE_ARRAY
 */

/**
 * LigmaArray:
 * @data: (array length=length): pointer to the array's data.
 * @length:      length of @data, in bytes.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _LigmaArray LigmaArray;

struct _LigmaArray
{
  guint8   *data;
  gsize     length;
  gboolean  static_data;
};

LigmaArray * ligma_array_new  (const guint8    *data,
                             gsize            length,
                             gboolean         static_data);
LigmaArray * ligma_array_copy (const LigmaArray *array);
void        ligma_array_free (LigmaArray       *array);

#define LIGMA_TYPE_ARRAY               (ligma_array_get_type ())
#define LIGMA_VALUE_HOLDS_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_ARRAY))

GType   ligma_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_ARRAY
 */

#define LIGMA_TYPE_PARAM_ARRAY           (ligma_param_array_get_type ())
#define LIGMA_PARAM_SPEC_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_ARRAY, LigmaParamSpecArray))
#define LIGMA_IS_PARAM_SPEC_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_ARRAY))

typedef struct _LigmaParamSpecArray LigmaParamSpecArray;

struct _LigmaParamSpecArray
{
  GParamSpecBoxed parent_instance;
};

GType        ligma_param_array_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_array     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        GParamFlags   flags);


/*
 * LIGMA_TYPE_UINT8_ARRAY
 */

#define LIGMA_TYPE_UINT8_ARRAY               (ligma_uint8_array_get_type ())
#define LIGMA_VALUE_HOLDS_UINT8_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_UINT8_ARRAY))

GType   ligma_uint8_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_UINT8_ARRAY
 */

#define LIGMA_TYPE_PARAM_UINT8_ARRAY           (ligma_param_uint8_array_get_type ())
#define LIGMA_PARAM_SPEC_UINT8_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_UINT8_ARRAY, LigmaParamSpecUInt8Array))
#define LIGMA_IS_PARAM_SPEC_UINT8_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_UINT8_ARRAY))

typedef struct _LigmaParamSpecUInt8Array LigmaParamSpecUInt8Array;

struct _LigmaParamSpecUInt8Array
{
  LigmaParamSpecArray parent_instance;
};

GType          ligma_param_uint8_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * ligma_param_spec_uint8_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const guint8 * ligma_value_get_uint8_array        (const GValue *value);
guint8       * ligma_value_dup_uint8_array        (const GValue *value);
void           ligma_value_set_uint8_array        (GValue       *value,
                                                  const guint8 *data,
                                                  gsize         length);
void           ligma_value_set_static_uint8_array (GValue       *value,
                                                  const guint8 *data,
                                                  gsize         length);
void           ligma_value_take_uint8_array       (GValue       *value,
                                                  guint8       *data,
                                                  gsize         length);


/*
 * LIGMA_TYPE_INT32_ARRAY
 */

#define LIGMA_TYPE_INT32_ARRAY               (ligma_int32_array_get_type ())
#define LIGMA_VALUE_HOLDS_INT32_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_INT32_ARRAY))

GType   ligma_int32_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_INT32_ARRAY
 */

#define LIGMA_TYPE_PARAM_INT32_ARRAY           (ligma_param_int32_array_get_type ())
#define LIGMA_PARAM_SPEC_INT32_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_INT32_ARRAY, LigmaParamSpecInt32Array))
#define LIGMA_IS_PARAM_SPEC_INT32_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_INT32_ARRAY))

typedef struct _LigmaParamSpecInt32Array LigmaParamSpecInt32Array;

struct _LigmaParamSpecInt32Array
{
  LigmaParamSpecArray parent_instance;
};

GType          ligma_param_int32_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * ligma_param_spec_int32_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gint32 * ligma_value_get_int32_array        (const GValue *value);
gint32       * ligma_value_dup_int32_array        (const GValue *value);
void           ligma_value_set_int32_array        (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           ligma_value_set_static_int32_array (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           ligma_value_take_int32_array       (GValue       *value,
                                                  gint32       *data,
                                                  gsize         length);


/*
 * LIGMA_TYPE_FLOAT_ARRAY
 */

#define LIGMA_TYPE_FLOAT_ARRAY               (ligma_float_array_get_type ())
#define LIGMA_VALUE_HOLDS_FLOAT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_FLOAT_ARRAY))

GType   ligma_float_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_FLOAT_ARRAY
 */

#define LIGMA_TYPE_PARAM_FLOAT_ARRAY           (ligma_param_float_array_get_type ())
#define LIGMA_PARAM_SPEC_FLOAT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_FLOAT_ARRAY, LigmaParamSpecFloatArray))
#define LIGMA_IS_PARAM_SPEC_FLOAT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_FLOAT_ARRAY))

typedef struct _LigmaParamSpecFloatArray LigmaParamSpecFloatArray;

struct _LigmaParamSpecFloatArray
{
  LigmaParamSpecArray parent_instance;
};

GType           ligma_param_float_array_get_type   (void) G_GNUC_CONST;

GParamSpec    * ligma_param_spec_float_array       (const gchar  *name,
                                                   const gchar  *nick,
                                                   const gchar  *blurb,
                                                   GParamFlags   flags);

const gdouble * ligma_value_get_float_array        (const GValue  *value);
gdouble       * ligma_value_dup_float_array        (const GValue  *value);
void            ligma_value_set_float_array        (GValue        *value,
                                                   const gdouble *data,
                                                   gsize         length);
void            ligma_value_set_static_float_array (GValue        *value,
                                                   const gdouble *data,
                                                   gsize         length);
void            ligma_value_take_float_array       (GValue        *value,
                                                   gdouble       *data,
                                                   gsize         length);


/*
 * LIGMA_TYPE_RGB_ARRAY
 */

#define LIGMA_TYPE_RGB_ARRAY               (ligma_rgb_array_get_type ())
#define LIGMA_VALUE_HOLDS_RGB_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_RGB_ARRAY))

GType   ligma_rgb_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_RGB_ARRAY
 */

#define LIGMA_TYPE_PARAM_RGB_ARRAY           (ligma_param_rgb_array_get_type ())
#define LIGMA_PARAM_SPEC_RGB_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_RGB_ARRAY, LigmaParamSpecRGBArray))
#define LIGMA_IS_PARAM_SPEC_RGB_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_RGB_ARRAY))

typedef struct _LigmaParamSpecRGBArray LigmaParamSpecRGBArray;

struct _LigmaParamSpecRGBArray
{
  GParamSpecBoxed parent_instance;
};

GType           ligma_param_rgb_array_get_type   (void) G_GNUC_CONST;

GParamSpec    * ligma_param_spec_rgb_array       (const gchar   *name,
                                                 const gchar   *nick,
                                                 const gchar   *blurb,
                                                 GParamFlags    flags);

const LigmaRGB * ligma_value_get_rgb_array        (const GValue  *value);
LigmaRGB       * ligma_value_dup_rgb_array        (const GValue  *value);
void            ligma_value_set_rgb_array        (GValue        *value,
                                                 const LigmaRGB *data,
                                                 gsize          length);
void            ligma_value_set_static_rgb_array (GValue        *value,
                                                 const LigmaRGB *data,
                                                 gsize          length);
void            ligma_value_take_rgb_array       (GValue        *value,
                                                 LigmaRGB       *data,
                                                 gsize          length);


/*
 * LIGMA_TYPE_OBJECT_ARRAY
 */

/**
 * LigmaObjectArray:
 * @object_type: #GType of the contained objects.
 * @data: (array length=length): pointer to the array's data.
 * @length:      length of @data, in number of objects.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _LigmaObjectArray LigmaObjectArray;

struct _LigmaObjectArray
{
  GType     object_type;
  GObject **data;
  gsize     length;
  gboolean  static_data;
};

LigmaObjectArray * ligma_object_array_new  (GType                  object_type,
                                          GObject              **data,
                                          gsize                  length,
                                          gboolean               static_data);
LigmaObjectArray * ligma_object_array_copy (const LigmaObjectArray  *array);
void              ligma_object_array_free (LigmaObjectArray        *array);

#define LIGMA_TYPE_OBJECT_ARRAY               (ligma_object_array_get_type ())
#define LIGMA_VALUE_HOLDS_OBJECT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_OBJECT_ARRAY))

GType   ligma_object_array_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_OBJECT_ARRAY
 */

#define LIGMA_TYPE_PARAM_OBJECT_ARRAY           (ligma_param_object_array_get_type ())
#define LIGMA_PARAM_SPEC_OBJECT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_OBJECT_ARRAY, LigmaParamSpecObjectArray))
#define LIGMA_IS_PARAM_SPEC_OBJECT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_OBJECT_ARRAY))

typedef struct _LigmaParamSpecObjectArray LigmaParamSpecObjectArray;

struct _LigmaParamSpecObjectArray
{
  GParamSpecBoxed parent_instance;

  GType           object_type;
};

GType         ligma_param_object_array_get_type   (void) G_GNUC_CONST;

GParamSpec  * ligma_param_spec_object_array       (const gchar   *name,
                                                  const gchar   *nick,
                                                  const gchar   *blurb,
                                                  GType          object_type,
                                                  GParamFlags    flags);

GObject    ** ligma_value_get_object_array        (const GValue  *value);
GObject    ** ligma_value_dup_object_array        (const GValue  *value);
void          ligma_value_set_object_array        (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);
void          ligma_value_set_static_object_array (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);
void          ligma_value_take_object_array       (GValue        *value,
                                                  GType          object_type,
                                                  GObject      **data,
                                                  gsize          length);


G_END_DECLS

#endif  /*  __LIGMA_PARAM_SPECS_H__  */
