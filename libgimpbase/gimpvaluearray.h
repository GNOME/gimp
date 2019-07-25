/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpvaluearray.h ported from GValueArray
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_VALUE_ARRAY_H__
#define __GIMP_VALUE_ARRAY_H__

G_BEGIN_DECLS

/**
 * GIMP_TYPE_VALUE_ARRAY:
 *
 * The type ID of the "GimpValueArray" type which is a boxed type,
 * used to pass around pointers to GimpValueArrays.
 *
 * Since: 2.10
 */
#define GIMP_TYPE_VALUE_ARRAY (gimp_value_array_get_type ())


GType            gimp_value_array_get_type (void) G_GNUC_CONST;

GimpValueArray * gimp_value_array_new      (gint                  n_prealloced);

GimpValueArray * gimp_value_array_ref      (GimpValueArray       *value_array);
void             gimp_value_array_unref    (GimpValueArray       *value_array);

gint             gimp_value_array_length   (const GimpValueArray *value_array);

GValue         * gimp_value_array_index    (const GimpValueArray *value_array,
                                            gint                  index);

GimpValueArray * gimp_value_array_prepend  (GimpValueArray       *value_array,
                                            const GValue         *value);
GimpValueArray * gimp_value_array_append   (GimpValueArray       *value_array,
                                            const GValue         *value);
GimpValueArray * gimp_value_array_insert   (GimpValueArray       *value_array,
                                            gint                  index,
                                            const GValue         *value);

GimpValueArray * gimp_value_array_remove   (GimpValueArray       *value_array,
                                            gint                  index);
void             gimp_value_array_truncate (GimpValueArray       *value_array,
                                            gint                  n_values);


/*
 * GIMP_TYPE_PARAM_VALUE_ARRAY
 */

#define GIMP_TYPE_PARAM_VALUE_ARRAY           (gimp_param_value_array_get_type ())
#define GIMP_IS_PARAM_SPEC_VALUE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_VALUE_ARRAY))
#define GIMP_PARAM_SPEC_VALUE_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_VALUE_ARRAY, GimpParamSpecValueArray))

typedef struct _GimpParamSpecValueArray GimpParamSpecValueArray;

/**
 * GimpParamSpecValueArray:
 * @parent_instance:  private #GParamSpec portion
 * @element_spec:     the #GParamSpec of the array elements
 * @fixed_n_elements: default length of the array
 *
 * A #GParamSpec derived structure that contains the meta data for
 * value array properties.
 **/
struct _GimpParamSpecValueArray
{
  GParamSpec  parent_instance;
  GParamSpec *element_spec;
  gint        fixed_n_elements;
};

GType        gimp_param_value_array_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_value_array     (const gchar    *name,
                                              const gchar    *nick,
                                              const gchar    *blurb,
                                              GParamSpec     *element_spec,
                                              GParamFlags     flags);


G_END_DECLS

#endif /* __GIMP_VALUE_ARRAY_H__ */
