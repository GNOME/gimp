/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedure-params.h
 * Copyright (C) 2019  Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PROCEDURE_PARAMS_H__
#define __GIMP_PROCEDURE_PARAMS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * SECTION: gimpprocedure-params
 * @title: GimpProcedure-params
 * @short_description: Macros and defines to add procedure arguments
 *                     and return values.
 *
 * Macros and defines to add procedure arguments and return values.
 **/


/*  boolean  */

#define GIMP_VALUES_GET_BOOLEAN(args, n) \
  g_value_get_boolean (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_BOOLEAN(args, n, value) \
  g_value_set_boolean (gimp_value_array_index (args, n), value)


/*  int  */

#define GIMP_VALUES_GET_INT(args, n) \
  g_value_get_int (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_INT(args, n, value) \
  g_value_set_int (gimp_value_array_index (args, n), value)


/*  uint  */

#define GIMP_VALUES_GET_UINT(args, n) \
  g_value_get_uint (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_UINT(args, n, value) \
  g_value_set_uint (gimp_value_array_index (args, n), value)


/* uchar  */

#define GIMP_VALUES_GET_UCHAR(args, n) \
  g_value_get_uchar (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_UCHAR(args, n, value) \
  g_value_set_uchar (gimp_value_array_index (args, n), value)

/*  double  */

#define GIMP_VALUES_GET_DOUBLE(args, n) \
  g_value_get_double (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_DOUBLE(args, n, value) \
  g_value_set_double (gimp_value_array_index (args, n), value)


/*  enum  */

#define GIMP_VALUES_GET_ENUM(args, n) \
  g_value_get_enum (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_ENUM(args, n, value) \
  g_value_set_enum (gimp_value_array_index (args, n), value)


/*  choice  */

#define GIMP_VALUES_GET_CHOICE(args, n) \
  g_value_get_int (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_CHOICE(args, n, value) \
  g_value_set_int (gimp_value_array_index (args, n), value)


/*  string  */

#define GIMP_VALUES_GET_STRING(args, n) \
  g_value_get_string (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_STRING(args, n) \
  g_value_dup_string (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_STRING(args, n, value) \
  g_value_set_string (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_STRING(args, n, value) \
  g_value_take_string (gimp_value_array_index (args, n), value)


/*  color  */

/* TODO:
 * 1. has_alpha is currently bogus and doesn't do anything yet.
 * 2. Also wouldn't none_ok be useful for color arguments (accepting a NULL
 *    GeglColor)?
 * 3. GEGL also provides a gegl_param_spec_color_from_string() allowing to
 *    initialize the default with a list of standard colors. Wouldn't it be
 *    interesting to also have this?
 */

#define GIMP_VALUES_GET_COLOR(args, n, value) \
  g_value_get_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_SET_COLOR(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  parasite  */

#define GIMP_VALUES_GET_PARASITE(args, n) \
  g_value_get_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_PARASITE(args, n) \
  g_value_dup_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_PARASITE(args, n, value) \
  g_value_set_boxed (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_PARASITE(args, n, value)                \
  g_value_take_boxed (gimp_value_array_index (args, n), value)


/*  param  */

#define GIMP_VALUES_GET_PARAM(args, n) \
  g_value_get_param (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_PARAM(args, n) \
  g_value_dup_param (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_PARAM(args, n, value) \
  g_value_set_param (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_PARAM(args, n, value)                \
  g_value_take_param (gimp_value_array_index (args, n), value)


/*  bytes  */

#define GIMP_VALUES_GET_BYTES(args, n) \
  g_value_get_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_BYTES(args, n) \
  g_value_dup_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_BYTES(args, n, value) \
  g_value_set_boxed (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_BYTES(args, n, value, length) \
  g_value_take_boxed (gimp_value_array_index (args, n), value)


/*  int32 array  */

#define GIMP_VALUES_GET_INT32_ARRAY(args, n, length) \
  gimp_value_get_int32_array (gimp_value_array_index (args, n), length)

#define GIMP_VALUES_DUP_INT32_ARRAY(args, n, length) \
  gimp_value_dup_int32_array (gimp_value_array_index (args, n), length)

#define GIMP_VALUES_SET_INT32_ARRAY(args, n, value, length) \
  gimp_value_set_int32_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_INT32_ARRAY(args, n, value, length) \
  gimp_value_take_int32_array (gimp_value_array_index (args, n), value, length)


/*  double array  */

#define GIMP_VALUES_GET_DOUBLE_ARRAY(args, n, length) \
  gimp_value_get_double_array (gimp_value_array_index (args, n), length)

#define GIMP_VALUES_DUP_DOUBLE_ARRAY(args, n, length) \
  gimp_value_dup_double_array (gimp_value_array_index (args, n), length)

#define GIMP_VALUES_SET_DOUBLE_ARRAY(args, n, value, length) \
  gimp_value_set_double_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_DOUBLE_ARRAY(args, n, value, length) \
  gimp_value_take_double_array (gimp_value_array_index (args, n), value, length)


/*  string array (strv)  */

#define GIMP_VALUES_GET_STRV(args, n) \
  g_value_get_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_STRV(args, n) \
  g_value_dup_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_STRV(args, n, value) \
  g_value_set_boxed (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_STRV(args, n, value) \
  g_value_take_boxed (gimp_value_array_index (args, n), value)


/*  object array  */

#define GIMP_VALUES_GET_CORE_OBJECT_ARRAY(args, n) \
  (gpointer) g_value_get_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_CORE_OBJECT_ARRAY(args, n) \
  (gpointer) g_value_dup_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_CORE_OBJECT_ARRAY(args, n, value) \
  g_value_set_boxed (gimp_value_array_index (args, n), (gconstpointer) value)

#define GIMP_VALUES_TAKE_CORE_OBJECT_ARRAY(args, n, value) \
  g_value_take_boxed (gimp_value_array_index (args, n), (gconstpointer) value)


/*  display  */

#define GIMP_VALUES_GET_DISPLAY(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_DISPLAY_ID(args, n) \
  gimp_display_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_DISPLAY(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  image  */

#define GIMP_VALUES_GET_IMAGE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_IMAGE_ID(args, n) \
  gimp_image_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_IMAGE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  item  */

#define GIMP_VALUES_GET_ITEM(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_ITEM_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_ITEM(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  drawable  */

#define GIMP_VALUES_GET_DRAWABLE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_DRAWABLE_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_DRAWABLE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  layer */

#define GIMP_VALUES_GET_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  text layer */

#define GIMP_VALUES_GET_TEXT_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_TEXT_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_TEXT_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  vector layer */

#define GIMP_VALUES_GET_VECTOR_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_VECTOR_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_VECTOR_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  link layer */

#define GIMP_VALUES_GET_LINK_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_LINK_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_LINK_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  rasterizable  */

#define GIMP_VALUES_GET_RASTERIZABLE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_RASTERIZABLE_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_RASTERIZABLE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  group layer */

#define GIMP_VALUES_GET_GROUP_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_GROUP_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_GROUP_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  channel  */

#define GIMP_VALUES_GET_CHANNEL(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_CHANNEL_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_CHANNEL(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  layer mask  */

#define GIMP_VALUES_GET_LAYER_MASK(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_LAYER_MASK_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_LAYER_MASK(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  selection  */

#define GIMP_VALUES_GET_SELECTION(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_SELECTION_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_SELECTION(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  path  */

#define GIMP_VALUES_GET_PATH(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_PATH_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_PATH(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  Drawable Filter  */

#define GIMP_VALUES_GET_DRAWABLE_FILTER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_DRAWABLE_FILTER_ID(args, n) \
  gimp_drawable_filter_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_DRAWABLE_FILTER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  file  */

#define GIMP_VALUES_GET_FILE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_FILE(args, n) \
  g_value_dup_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_FILE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_FILE(args, n, value) \
  g_value_take_object (gimp_value_array_index (args, n), value)


/*  Resource  */

#define GIMP_VALUES_GET_RESOURCE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_RESOURCE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_GET_BRUSH(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_BRUSH(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_GET_FONT(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_FONT(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_GET_GRADIENT(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_GRADIENT(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_GET_PALETTE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_PALETTE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_GET_PATTERN(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_PATTERN(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  Unit  */

#define GIMP_VALUES_GET_UNIT(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))
#define GIMP_VALUES_SET_UNIT(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


void gimp_procedure_add_boolean_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       value,
                                                        GParamFlags    flags);
void gimp_procedure_add_boolean_aux_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       value,
                                                        GParamFlags    flags);
void gimp_procedure_add_boolean_return_value           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       value,
                                                        GParamFlags    flags);

void gimp_procedure_add_int_argument                   (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gint           min,
                                                        gint           max,
                                                        gint           value,
                                                        GParamFlags    flags);
void gimp_procedure_add_int_aux_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gint           min,
                                                        gint           max,
                                                        gint           value,
                                                        GParamFlags    flags);
void gimp_procedure_add_int_return_value               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gint           min,
                                                        gint           max,
                                                        gint           value,
                                                        GParamFlags    flags);

void gimp_procedure_add_uint_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        guint          min,
                                                        guint          max,
                                                        guint          value,
                                                        GParamFlags    flags);
void gimp_procedure_add_uint_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        guint          min,
                                                        guint          max,
                                                        guint          value,
                                                        GParamFlags    flags);
void gimp_procedure_add_uint_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        guint          min,
                                                        guint          max,
                                                        guint          value,
                                                        GParamFlags    flags);

void gimp_procedure_add_unit_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       show_pixels,
                                                        gboolean       show_percent,
                                                        GimpUnit      *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_unit_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       show_pixels,
                                                        gboolean       show_percent,
                                                        GimpUnit      *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_unit_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       show_pixels,
                                                        gboolean       show_percent,
                                                        GimpUnit      *value,
                                                        GParamFlags    flags);

void gimp_procedure_add_double_argument                (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gdouble        min,
                                                        gdouble        max,
                                                        gdouble        value,
                                                        GParamFlags    flags);
void gimp_procedure_add_double_aux_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gdouble        min,
                                                        gdouble        max,
                                                        gdouble        value,
                                                        GParamFlags    flags);
void gimp_procedure_add_double_return_value            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gdouble        min,
                                                        gdouble        max,
                                                        gdouble        value,
                                                        GParamFlags    flags);

void gimp_procedure_add_enum_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          enum_type,
                                                        gint           value,
                                                        GParamFlags    flags);
void gimp_procedure_add_enum_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          enum_type,
                                                        gint           value,
                                                        GParamFlags    flags);
void gimp_procedure_add_enum_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          enum_type,
                                                        gint           value,
                                                        GParamFlags    flags);

void gimp_procedure_add_choice_argument                (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpChoice    *choice,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_choice_aux_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpChoice    *choice,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_choice_return_value            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpChoice    *choice,
                                                        const gchar   *value,
                                                        GParamFlags    flags);

void gimp_procedure_add_string_argument                (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_string_aux_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_string_return_value            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        const gchar   *value,
                                                        GParamFlags    flags);

void gimp_procedure_add_color_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        GeglColor     *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_color_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        GeglColor     *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_color_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        GeglColor     *value,
                                                        GParamFlags    flags);

void gimp_procedure_add_color_from_string_argument     (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_color_from_string_aux_argument (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        const gchar   *value,
                                                        GParamFlags    flags);
void gimp_procedure_add_color_from_string_return_value (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       has_alpha,
                                                        const gchar   *value,
                                                        GParamFlags    flags);

void gimp_procedure_add_parasite_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_parasite_aux_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_parasite_return_value          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_param_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          param_type,
                                                        GParamFlags    flags);
void gimp_procedure_add_param_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          param_type,
                                                        GParamFlags    flags);
void gimp_procedure_add_param_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          param_type,
                                                        GParamFlags    flags);

void gimp_procedure_add_bytes_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_bytes_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_bytes_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_int32_array_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_int32_array_aux_argument       (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_int32_array_return_value       (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_double_array_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_double_array_aux_argument      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_double_array_return_value      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_string_array_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_string_array_aux_argument      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);
void gimp_procedure_add_string_array_return_value      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_core_object_array_argument     (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          object_type,
                                                        GParamFlags    flags);
void gimp_procedure_add_core_object_array_aux_argument (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          object_type,
                                                        GParamFlags    flags);
void gimp_procedure_add_core_object_array_return_value (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GType          object_type,
                                                        GParamFlags    flags);

void gimp_procedure_add_display_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_display_aux_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_display_return_value           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_image_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_image_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_image_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_item_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_item_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_item_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_drawable_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_drawable_aux_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_drawable_return_value          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_layer_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_layer_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_layer_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_text_layer_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_text_layer_aux_argument        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_text_layer_return_value        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_vector_layer_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_vector_layer_aux_argument      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_vector_layer_return_value      (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_link_layer_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_link_layer_aux_argument        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_link_layer_return_value        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_group_layer_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_group_layer_aux_argument       (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_group_layer_return_value       (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_channel_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_channel_aux_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_channel_return_value           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_layer_mask_argument            (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_layer_mask_aux_argument        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_layer_mask_return_value        (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_selection_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_selection_aux_argument         (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_selection_return_value         (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_path_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_path_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);
void gimp_procedure_add_path_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GParamFlags    flags);

void gimp_procedure_add_file_argument                  (GimpProcedure         *procedure,
                                                        const gchar           *name,
                                                        const gchar           *nick,
                                                        const gchar           *blurb,
                                                        GimpFileChooserAction  action,
                                                        gboolean               none_ok,
                                                        GFile                 *default_file,
                                                        GParamFlags            flags);
void gimp_procedure_add_file_aux_argument              (GimpProcedure         *procedure,
                                                        const gchar           *name,
                                                        const gchar           *nick,
                                                        const gchar           *blurb,
                                                        GimpFileChooserAction  action,
                                                        gboolean               none_ok,
                                                        GFile                 *default_file,
                                                        GParamFlags            flags);
void gimp_procedure_add_file_return_value              (GimpProcedure         *procedure,
                                                        const gchar           *name,
                                                        const gchar           *nick,
                                                        const gchar           *blurb,
                                                        GParamFlags            flags);

void gimp_procedure_add_resource_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpResource  *default_value,
                                                        GParamFlags    flags);
void gimp_procedure_add_resource_aux_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpResource  *default_value,
                                                        GParamFlags    flags);
void gimp_procedure_add_resource_return_value          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_brush_argument                 (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpBrush     *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_brush_aux_argument             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpBrush     *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_brush_return_value             (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_font_argument                  (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpFont      *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_font_aux_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpFont      *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_font_return_value              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_gradient_argument              (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpGradient  *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_gradient_aux_argument          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpGradient  *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_gradient_return_value          (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_palette_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpPalette   *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_palette_aux_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpPalette   *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_palette_return_value           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);

void gimp_procedure_add_pattern_argument               (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        gboolean       none_ok,
                                                        GimpPattern   *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_pattern_aux_argument           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GimpPattern   *default_value,
                                                        gboolean       default_to_context,
                                                        GParamFlags    flags);
void gimp_procedure_add_pattern_return_value           (GimpProcedure *procedure,
                                                        const gchar   *name,
                                                        const gchar   *nick,
                                                        const gchar   *blurb,
                                                        GParamFlags    flags);


G_END_DECLS

#endif /* __GIMP_PROCEDURE_PARAMS_H__ */
