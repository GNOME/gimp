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


#define GIMP_PROC_ARG_BOOLEAN(class, name, nick, blurb, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_boolean (name, nick, blurb,\
                               default,\
                               flags))

#define GIMP_PROC_VAL_BOOLEAN(class, name, nick, blurb, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_boolean (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_PROC_ARG_INT(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_int (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_VAL_INT(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_int (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_ARG_UINT(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_uint (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_VAL_UINT(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_uint (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_ARG_UCHAR(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_uchar (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_VAL_UCHAR(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_uchar (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_ARG_UNIT(class, name, nick, blurb, pixels, percent, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_unit (name, nick, blurb,\
                               pixels, percent, default,\
                               flags))

#define GIMP_PROC_VAL_UNIT(class, name, nick, blurb, pixels, percent, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_unit (name, nick, blurb,\
                                   pixels, percent, default,\
                                   flags))

#define GIMP_PROC_ARG_DOUBLE(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_double (name, nick, blurb,\
                               min, max, default,\
                               flags))

#define GIMP_PROC_VAL_DOUBLE(class, name, nick, blurb, min, max, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_double (name, nick, blurb,\
                                   min, max, default,\
                                   flags))

#define GIMP_PROC_ARG_ENUM(class, name, nick, blurb, enum_type, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_enum (name, nick, blurb,\
                               enum_type, default,\
                               flags))

#define GIMP_PROC_VAL_ENUM(class, name, nick, blurb, enum_type, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_enum (name, nick, blurb,\
                                   enum_type, default,\
                                   flags))

#define GIMP_PROC_ARG_STRING(class, name, nick, blurb, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               g_param_spec_string (name, nick, blurb,\
                               default,\
                               flags))

#define GIMP_PROC_VAL_STRING(class, name, nick, blurb, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   g_param_spec_string (name, nick, blurb,\
                                   default,\
                                   flags))

#define GIMP_PROC_ARG_RGB(class, name, nick, blurb, has_alpha, default, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_rgb (name, nick, blurb,\
                               has_alpha, default, \
                               flags))

#define GIMP_PROC_VAL_RGB(class, name, nick, blurb, has_alpha, default, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_rgb (name, nick, blurb,\
                                   has_alpha, default, \
                                   flags))

#define GIMP_PROC_ARG_UINT8_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_uint8_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_UINT8_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_uint8_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_INT16_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_int16_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_INT16_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_int16_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_INT32_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_int32_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_INT32_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_int32_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_FLOAT_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_float_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_FLOAT_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_float_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_STRING_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_string_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_STRING_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_string_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_RGB_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_rgb_array (name, nick, blurb,\
                               flags))

#define GIMP_PROC_VAL_RGB_ARRAY(class, name, nick, blurb, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_rgb_array (name, nick, blurb,\
                                   flags))

#define GIMP_PROC_ARG_DISPLAY(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_display_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_DISPLAY(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_display_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_IMAGE(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_image_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_IMAGE(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_image_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_ITEM(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_item_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_ITEM(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_item_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_DRAWABLE(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_drawable_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_DRAWABLE(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_drawable_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_LAYER(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_layer_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_LAYER(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_layer_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_CHANNEL(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_channel_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_CHANNEL(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_channel_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_LAYER_MASK(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_layer_mask_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_LAYER_MASK(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_layer_mask_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_SELECTION(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_selection_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_SELECTION(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_selection_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))

#define GIMP_PROC_ARG_VECTORS(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_argument (procedure,\
                               gimp_param_spec_vectors_id (name, nick, blurb,\
                               none_ok, \
                               flags))

#define GIMP_PROC_VAL_VECTORS(class, name, nick, blurb, none_ok, flags) \
  gimp_procedure_add_return_value (procedure,\
                                   gimp_param_spec_vectors_id (name, nick, blurb,\
                                   none_ok, \
                                   flags))


G_END_DECLS

#endif /* __GIMP_PROCEDURE_PARAMS_H__ */
