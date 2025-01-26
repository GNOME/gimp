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
 * GIMP_TYPE_PARAM_OBJECT
 */

#define GIMP_TYPE_PARAM_OBJECT                  (gimp_param_object_get_type ())
#define GIMP_PARAM_SPEC_OBJECT(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_OBJECT, GimpParamSpecObject))
#define GIMP_PARAM_SPEC_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PARAM_OBJECT, GimpParamSpecObjectClass))
#define GIMP_IS_PARAM_SPEC_OBJECT(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_OBJECT))
#define GIMP_IS_PARAM_SPEC_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PARAM_OBJECT))
#define GIMP_PARAM_SPEC_OBJECT_GET_CLASS(pspec) (G_TYPE_INSTANCE_GET_CLASS ((pspec), G_TYPE_PARAM_OBJECT, GimpParamSpecObjectClass))


typedef struct _GimpParamSpecObject      GimpParamSpecObject;
typedef struct _GimpParamSpecObjectClass GimpParamSpecObjectClass;

struct _GimpParamSpecObject
{
  GParamSpecObject  parent_instance;

  /*< private >*/
  GObject          *_default_value;
  gboolean          _has_default;
};

struct _GimpParamSpecObjectClass
{
  /* XXX: vapigen fails with the following error without the private
   * comment:
   * > error: The type name `GLib.ParamSpecClass' could not be found
   * Not sure why it doesn't search for GObject.ParamSpecClass instead.
   * Anyway putting it private is good enough and hides the parent_class
   * to bindings.
   */
  /*< private >*/
  GParamSpecClass   parent_class;

  GParamSpec      * (* duplicate)   (GParamSpec *pspec);
  GObject         * (* get_default) (GParamSpec *pspec);

  /* Padding for future expansion */
  void              (*_gimp_reserved0) (void);
  void              (*_gimp_reserved1) (void);
  void              (*_gimp_reserved2) (void);
  void              (*_gimp_reserved3) (void);
  void              (*_gimp_reserved4) (void);
  void              (*_gimp_reserved5) (void);
  void              (*_gimp_reserved6) (void);
  void              (*_gimp_reserved7) (void);
  void              (*_gimp_reserved8) (void);
  void              (*_gimp_reserved9) (void);
};

GType        gimp_param_object_get_type         (void) G_GNUC_CONST;

GObject    * gimp_param_spec_object_get_default (GParamSpec  *pspec);
void         gimp_param_spec_object_set_default (GParamSpec  *pspec,
                                                 GObject     *default_value);
gboolean     gimp_param_spec_object_has_default (GParamSpec  *pspec);

GParamSpec * gimp_param_spec_object_duplicate   (GParamSpec  *pspec);


/*
 * GIMP_TYPE_PARAM_FILE
 */

#define GIMP_VALUE_HOLDS_FILE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_FILE))

#define GIMP_TYPE_PARAM_FILE           (gimp_param_file_get_type ())
#define GIMP_IS_PARAM_SPEC_FILE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FILE))


GType                   gimp_param_file_get_type          (void) G_GNUC_CONST;

GParamSpec            * gimp_param_spec_file              (const gchar           *name,
                                                           const gchar           *nick,
                                                           const gchar           *blurb,
                                                           GimpFileChooserAction  action,
                                                           gboolean               none_ok,
                                                           GFile                 *default_value,
                                                           GParamFlags            flags);

GimpFileChooserAction   gimp_param_spec_file_get_action   (GParamSpec            *pspec);
void                    gimp_param_spec_file_set_action   (GParamSpec            *pspec,
                                                           GimpFileChooserAction  action);
gboolean                gimp_param_spec_file_none_allowed (GParamSpec            *pspec);



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
#define GIMP_IS_PARAM_SPEC_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ARRAY))

GType        gimp_param_array_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_array     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_INT32_ARRAY
 */

#define        GIMP_TYPE_INT32_ARRAY               (gimp_int32_array_get_type ())
#define        GIMP_VALUE_HOLDS_INT32_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT32_ARRAY))

GType          gimp_int32_array_get_type           (void) G_GNUC_CONST;

const gint32 * gimp_int32_array_get_values         (GimpArray    *array,
                                                    gsize        *length);
void           gimp_int32_array_set_values         (GimpArray    *array,
                                                    const gint32 *values,
                                                    gsize         length,
                                                    gboolean      static_data);


/*
 * GIMP_TYPE_PARAM_INT32_ARRAY
 */

#define GIMP_TYPE_PARAM_INT32_ARRAY           (gimp_param_int32_array_get_type ())
#define GIMP_IS_PARAM_SPEC_INT32_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT32_ARRAY))

GType          gimp_param_int32_array_get_type   (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int32_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gint32 * gimp_value_get_int32_array        (const GValue *value,
                                                  gsize        *length);
gint32       * gimp_value_dup_int32_array        (const GValue *value,
                                                  gsize        *length);
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
 * GIMP_TYPE_DOUBLE_ARRAY
 */

#define         GIMP_TYPE_DOUBLE_ARRAY               (gimp_double_array_get_type ())
#define         GIMP_VALUE_HOLDS_DOUBLE_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_DOUBLE_ARRAY))

GType           gimp_double_array_get_type           (void) G_GNUC_CONST;

const gdouble * gimp_double_array_get_values         (GimpArray     *array,
                                                      gsize         *length);
void            gimp_double_array_set_values         (GimpArray     *array,
                                                      const gdouble *values,
                                                      gsize         length,
                                                      gboolean      static_data);


/*
 * GIMP_TYPE_PARAM_DOUBLE_ARRAY
 */

#define GIMP_TYPE_PARAM_DOUBLE_ARRAY           (gimp_param_double_array_get_type ())
#define GIMP_IS_PARAM_SPEC_DOUBLE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DOUBLE_ARRAY))

GType           gimp_param_double_array_get_type   (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_double_array       (const gchar  *name,
                                                    const gchar  *nick,
                                                    const gchar  *blurb,
                                                    GParamFlags   flags);

const gdouble * gimp_value_get_double_array        (const GValue  *value,
                                                    gsize         *length);
gdouble       * gimp_value_dup_double_array        (const GValue  *value,
                                                    gsize         *length);
void            gimp_value_set_double_array        (GValue        *value,
                                                    const gdouble *data,
                                                    gsize         length);
void            gimp_value_set_static_double_array (GValue        *value,
                                                    const gdouble *data,
                                                    gsize         length);
void            gimp_value_take_double_array       (GValue        *value,
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
 * GIMP_TYPE_CORE_OBJECT_ARRAY
 */

/**
 * GimpCoreObjectArray:
 *
 * A boxed type which is nothing more than an alias to a %NULL-terminated array
 * of [class@GObject.Object]. No reference is being hold on contents
 * because core objects are owned by `libgimp`.
 *
 * The reason of existence for this alias is to have common arrays of
 * objects as a boxed type easy to use as plug-in's procedure argument.
 *
 * You should never have to interact with this type directly, though
 * [func@Gimp.core_object_array_get_length] might be convenient.
 *
 * Since: 3.0
 */
typedef GObject**                                 GimpCoreObjectArray;

#define GIMP_TYPE_CORE_OBJECT_ARRAY               (gimp_core_object_array_get_type ())
#define GIMP_VALUE_HOLDS_CORE_OBJECT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_CORE_OBJECT_ARRAY))

GType gimp_core_object_array_get_type   (void) G_GNUC_CONST;
gsize gimp_core_object_array_get_length (GObject **array);


/*
 * GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY
 */

#define GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY           (gimp_param_core_object_array_get_type ())
#define GIMP_IS_PARAM_SPEC_CORE_OBJECT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CORE_OBJECT_ARRAY))

GType         gimp_param_core_object_array_get_type             (void) G_GNUC_CONST;

GParamSpec  * gimp_param_spec_core_object_array                 (const gchar   *name,
                                                                 const gchar   *nick,
                                                                 const gchar   *blurb,
                                                                 GType          object_type,
                                                                 GParamFlags    flags);

GType         gimp_param_spec_core_object_array_get_object_type (GParamSpec *pspec);


G_END_DECLS

#endif  /*  __GIMP_PARAM_SPECS_H__  */
