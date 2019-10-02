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

#define GIMP_PROC_ARG_BOOLEAN(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_boolean (name, nick, blurb,\
                               default,\
                               flags))

#define GIMP_PROC_AUX_ARG_BOOLEAN(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_boolean (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_PROC_VAL_BOOLEAN(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_boolean (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_VALUES_GET_BOOLEAN(args, n) \
  g_value_get_boolean (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_BOOLEAN(args, n, value) \
  g_value_set_boolean (gimp_value_array_index (args, n), value)


/*  int  */

#define GIMP_PROC_ARG_INT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_int (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_INT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_int (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_VAL_INT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_int (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_VALUES_GET_INT(args, n) \
  g_value_get_int (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_INT(args, n, value) \
  g_value_set_int (gimp_value_array_index (args, n), value)


/*  uint  */

#define GIMP_PROC_ARG_UINT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_uint (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_UINT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_uint (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_VAL_UINT(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_uint (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_VALUES_GET_UINT(args, n) \
  g_value_get_uint (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_UINT(args, n, value) \
  g_value_set_uint (gimp_value_array_index (args, n), value)


/* uchar  */

#define GIMP_PROC_ARG_UCHAR(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_uchar (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_UCHAR(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_uchar (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_VAL_UCHAR(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_uchar (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_VALUES_GET_UCHAR(args, n) \
  g_value_get_uchar (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_UCHAR(args, n, value) \
  g_value_set_uchar (gimp_value_array_index (args, n), value)


/*  unit  */

#define GIMP_PROC_ARG_UNIT(procedure, name, nick, blurb, pixels, percent, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_unit (name, nick, blurb,\
                               pixels, percent, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_UNIT(procedure, name, nick, blurb, pixels, percent, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_unit (name, nick, blurb,\
                                   pixels, percent, default,\
                                   flags))

#define GIMP_PROC_VAL_UNIT(procedure, name, nick, blurb, pixels, percent, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_unit (name, nick, blurb,\
                                   pixels, percent, default,\
                                   flags))


/*  double  */

#define GIMP_PROC_ARG_DOUBLE(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_double (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_DOUBLE(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_double (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_VAL_DOUBLE(procedure, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_double (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_VALUES_GET_DOUBLE(args, n) \
  g_value_get_double (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_DOUBLE(args, n, value) \
  g_value_set_double (gimp_value_array_index (args, n), value)


/*  enum  */

#define GIMP_PROC_ARG_ENUM(procedure, name, nick, blurb, enum_type, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_enum (name, nick, blurb,\
                               enum_type, default,\
                               flags))

#define GIMP_PROC_AUX_ARG_ENUM(procedure, name, nick, blurb, enum_type, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_enum (name, nick, blurb,\
                                   enum_type, default,\
                                   flags))

#define GIMP_PROC_VAL_ENUM(procedure, name, nick, blurb, enum_type, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_enum (name, nick, blurb,\
                                   enum_type, default,\
                                   flags))

#define GIMP_VALUES_GET_ENUM(args, n) \
  g_value_get_enum (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_ENUM(args, n, value) \
  g_value_set_enum (gimp_value_array_index (args, n), value)


/*  string  */

#define GIMP_PROC_ARG_STRING(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_string (name, nick, blurb,\
                               default,\
                               flags))

#define GIMP_PROC_AUX_ARG_STRING(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_string (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_PROC_VAL_STRING(procedure, name, nick, blurb, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_string (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_VALUES_GET_STRING(args, n) \
  g_value_get_string (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_STRING(args, n) \
  g_value_dup_string (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_STRING(args, n, value) \
  g_value_set_string (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_STRING(args, n, value) \
  g_value_take_string (gimp_value_array_index (args, n), value)


/*  rgb  */

#define GIMP_PROC_ARG_RGB(procedure, name, nick, blurb, has_alpha, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_rgb (name, nick, blurb,\
                               has_alpha, default, \
                               flags))

#define GIMP_PROC_AUX_ARG_RGB(procedure, name, nick, blurb, has_alpha, default, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_rgb (name, nick, blurb,\
                                   has_alpha, default, \
                                   flags))

#define GIMP_PROC_VAL_RGB(procedure, name, nick, blurb, has_alpha, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_rgb (name, nick, blurb,\
                                   has_alpha, default, \
                                   flags))

#define GIMP_VALUES_GET_RGB(args, n, value) \
  gimp_value_get_rgb (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_SET_RGB(args, n, value) \
  gimp_value_set_rgb (gimp_value_array_index (args, n), value)


/*  parasite  */

#define GIMP_PROC_ARG_PARASITE(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_parasite (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_PARASITE(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_parasite (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_PARASITE(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_parasite (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_PARASITE(args, n) \
  g_value_get_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_PARASITE(args, n) \
  g_value_dup_boxed (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_PARASITE(args, n, value) \
  g_value_set_boxed (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_PARASITE(args, n, value)                \
  g_value_take_boxed (gimp_value_array_index (args, n), value)


/*  param  */

#define GIMP_PROC_ARG_PARAM(procedure, name, nick, blurb, param_type, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_param (name, nick, blurb, param_type, \
                               flags))

#define GIMP_PROC_AUX_ARG_PARAM(procedure, name, nick, blurb, param_type, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_param (name, nick, blurb, param_type, \
                                   flags))

#define GIMP_PROC_VAL_PARAM(procedure, name, nick, blurb, param_type, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_param (name, nick, blurb, param_type, \
                                   flags))

#define GIMP_VALUES_GET_PARAM(args, n) \
  g_value_get_param (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_PARAM(args, n) \
  g_value_dup_param (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_PARAM(args, n, value) \
  g_value_set_param (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_PARAM(args, n, value)                \
  g_value_take_param (gimp_value_array_index (args, n), value)


/*  uint8 array  */

#define GIMP_PROC_ARG_UINT8_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_uint8_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_UINT8_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_uint8_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_UINT8_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_uint8_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_UINT8_ARRAY(args, n) \
  gimp_value_get_uint8_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_UINT8_ARRAY(args, n) \
  gimp_value_dup_uint8_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_UINT8_ARRAY(args, n, value, length) \
  gimp_value_set_uint8_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_UINT8_ARRAY(args, n, value, length) \
  gimp_value_take_uint8_array (gimp_value_array_index (args, n), value, length)


/*  int16 array  */

#define GIMP_PROC_ARG_INT16_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_int16_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_INT16_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_int16_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_INT16_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_int16_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_INT16_ARRAY(args, n) \
  gimp_value_get_int16_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_INT16_ARRAY(args, n) \
  gimp_value_dup_int16_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_INT16_ARRAY(args, n, value, length) \
  gimp_value_set_int16_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_INT16_ARRAY(args, n, value, length) \
  gimp_value_take_int16_array (gimp_value_array_index (args, n), value, length)


/*  int32 array  */

#define GIMP_PROC_ARG_INT32_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_int32_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_INT32_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_int32_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_INT32_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_int32_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_INT32_ARRAY(args, n) \
  gimp_value_get_int32_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_INT32_ARRAY(args, n) \
  gimp_value_dup_int32_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_INT32_ARRAY(args, n, value, length) \
  gimp_value_set_int32_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_INT32_ARRAY(args, n, value, length) \
  gimp_value_take_int32_array (gimp_value_array_index (args, n), value, length)


/*  float array  */

#define GIMP_PROC_ARG_FLOAT_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_float_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_FLOAT_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_float_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_FLOAT_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_float_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_FLOAT_ARRAY(args, n) \
  gimp_value_get_float_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_FLOAT_ARRAY(args, n) \
  gimp_value_dup_float_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_FLOAT_ARRAY(args, n, value, length) \
  gimp_value_set_float_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_FLOAT_ARRAY(args, n, value, length) \
  gimp_value_take_float_array (gimp_value_array_index (args, n), value, length)


/*  string array  */

#define GIMP_PROC_ARG_STRING_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_string_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_STRING_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_string_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_STRING_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_string_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_STRING_ARRAY(args, n) \
  gimp_value_get_string_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_STRING_ARRAY(args, n) \
  gimp_value_dup_string_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_STRING_ARRAY(args, n, value, length) \
  gimp_value_set_string_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_STRING_ARRAY(args, n, value, length) \
  gimp_value_take_string_array (gimp_value_array_index (args, n), value, length)


/*  rgb array  */

#define GIMP_PROC_ARG_RGB_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_rgb_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_AUX_ARG_RGB_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_rgb_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_VAL_RGB_ARRAY(procedure, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_rgb_array (name, nick, blurb,\
                                   flags))

#define GIMP_VALUES_GET_RGB_ARRAY(args, n) \
  gimp_value_get_rgb_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_RGB_ARRAY(args, n) \
  gimp_value_dup_rgb_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_RGB_ARRAY(args, n, value, length) \
  gimp_value_set_rgb_array (gimp_value_array_index (args, n), value, length)

#define GIMP_VALUES_TAKE_RGB_ARRAY(args, n, value, length) \
  gimp_value_take_rgb_array (gimp_value_array_index (args, n), value, length)


/*  object array  */

#define GIMP_PROC_ARG_OBJECT_ARRAY(procedure, name, nick, blurb, object_type, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_object_array (name, nick, blurb,\
                                                             object_type, flags))

#define GIMP_PROC_AUX_ARG_OBJECT_ARRAY(procedure, name, nick, blurb, object_type, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_object_array (name, nick, blurb,\
                                                                 object_type, flags))

#define GIMP_PROC_VAL_OBJECT_ARRAY(procedure, name, nick, blurb, object_type, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_object_array (name, nick, blurb,\
                                                                 object_type, flags))

#define GIMP_VALUES_GET_OBJECT_ARRAY(args, n) \
  (gpointer) gimp_value_get_object_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_OBJECT_ARRAY(args, n) \
  (gpointer) gimp_value_dup_object_array (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_OBJECT_ARRAY(args, n, object_type, value, length) \
  gimp_value_set_object_array (gimp_value_array_index (args, n),\
                               object_type, (gpointer) value, length)

#define GIMP_VALUES_TAKE_OBJECT_ARRAY(args, n, object_type, value, length) \
  gimp_value_take_object_array (gimp_value_array_index (args, n),\
                                object_type, (gpointer) value, length)


/*  display  */

#define GIMP_PROC_ARG_DISPLAY(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_display (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_DISPLAY(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_display (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_DISPLAY(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_display (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_DISPLAY(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_DISPLAY_ID(args, n) \
  gimp_display_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_DISPLAY(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  image  */

#define GIMP_PROC_ARG_IMAGE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_image (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_IMAGE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                               gimp_param_spec_image (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_VAL_IMAGE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_image (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_IMAGE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_IMAGE_ID(args, n) \
  gimp_image_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_IMAGE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  item  */

#define GIMP_PROC_ARG_ITEM(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_item (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_ITEM(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_item (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_ITEM(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_item (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_ITEM(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_ITEM_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_ITEM(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  drawable  */

#define GIMP_PROC_ARG_DRAWABLE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_drawable (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_DRAWABLE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_drawable (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_DRAWABLE(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_drawable (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_DRAWABLE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_DRAWABLE_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_DRAWABLE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  layer */

#define GIMP_PROC_ARG_LAYER(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_layer (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_LAYER(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_layer (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_LAYER(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_layer (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_LAYER(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_LAYER_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_LAYER(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  channel  */

#define GIMP_PROC_ARG_CHANNEL(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_channel (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_CHANNEL(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_channel (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_CHANNEL(procedure, name, nick, blurb, none_ok, flags)  \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_channe (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_CHANNEL(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_CHANNEL_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_CHANNEL(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  layer mask  */

#define GIMP_PROC_ARG_LAYER_MASK(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_layer_mask (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_LAYER_MASK(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_layer_mask (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_LAYER_MASK(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_layer_mask (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_LAYER_MASK(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_LAYER_MASK_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_LAYER_MASK(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  selection  */

#define GIMP_PROC_ARG_SELECTION(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_selection (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_SELECTION(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_selection (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_SELECTION(procedure, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_selection (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_SELECTION(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_SELECTION_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_SELECTION(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  vectors  */

#define GIMP_PROC_ARG_VECTORS(procedure, name, nick, blurb, none_ok, flags)  \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_vectors (name, nick, blurb,\
                               none_ok,\
                               flags))

#define GIMP_PROC_AUX_ARG_VECTORS(procedure, name, nick, blurb, none_ok, flags)  \
  gimp_procedure_add_aux_argument (procedure,\
                                   gimp_param_spec_vectors (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_PROC_VAL_VECTORS(procedure, name, nick, blurb, none_ok, flags)  \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_vectors (name, nick, blurb,\
                                   none_ok,\
                                   flags))

#define GIMP_VALUES_GET_VECTORS(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_GET_VECTORS_ID(args, n) \
  gimp_item_get_id (g_value_get_object (gimp_value_array_index (args, n)))

#define GIMP_VALUES_SET_VECTORS(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)


/*  file  */

#define GIMP_PROC_ARG_FILE(procedure, name, nick, blurb, flags)  \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_object (name, nick, blurb,\
                                                    G_TYPE_FILE,\
                                                    flags))

#define GIMP_PROC_AUX_ARG_FILE(procedure, name, nick, blurb, flags)  \
  gimp_procedure_add_aux_argument (procedure,\
                                   g_param_spec_object (name, nick, blurb,\
                                                        G_TYPE_FILE,\
                                                        flags))

#define GIMP_PROC_VAL_FILE(procedure, name, nick, blurb, none_ok, flags)  \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_object (name, nick, blurb,\
                                                        G_TYPE_FILE,\
                                                        flags))

#define GIMP_VALUES_GET_FILE(args, n) \
  g_value_get_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_DUP_FILE(args, n) \
  g_value_dup_object (gimp_value_array_index (args, n))

#define GIMP_VALUES_SET_FILE(args, n, value) \
  g_value_set_object (gimp_value_array_index (args, n), value)

#define GIMP_VALUES_TAKE_FILE(args, n, value) \
  g_value_take_object (gimp_value_array_index (args, n), value)


G_END_DECLS

#endif /* __GIMP_PROCEDURE_PARAMS_H__ */
