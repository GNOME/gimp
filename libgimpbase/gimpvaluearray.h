/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmavaluearray.h ported from GValueArray
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_VALUE_ARRAY_H__
#define __LIGMA_VALUE_ARRAY_H__

G_BEGIN_DECLS

/**
 * LIGMA_TYPE_VALUE_ARRAY:
 *
 * The type ID of the "LigmaValueArray" type which is a boxed type,
 * used to pass around pointers to LigmaValueArrays.
 *
 * Since: 2.10
 */
#define LIGMA_TYPE_VALUE_ARRAY (ligma_value_array_get_type ())


GType            ligma_value_array_get_type (void) G_GNUC_CONST;

LigmaValueArray * ligma_value_array_new      (gint                  n_prealloced);
LigmaValueArray * ligma_value_array_new_from_types
                                           (gchar               **error_msg,
                                            GType                 first_type,
                                            ...);
LigmaValueArray * ligma_value_array_new_from_types_valist
                                           (gchar               **error_msg,
                                            GType                 first_type,
                                            va_list               va_args);
LigmaValueArray * ligma_value_array_new_from_values
                                           (const GValue *values,
                                            gint          n_values);

LigmaValueArray * ligma_value_array_ref      (LigmaValueArray       *value_array);
void             ligma_value_array_unref    (LigmaValueArray       *value_array);

gint             ligma_value_array_length   (const LigmaValueArray *value_array);

GValue         * ligma_value_array_index    (const LigmaValueArray *value_array,
                                            gint                  index);

LigmaValueArray * ligma_value_array_prepend  (LigmaValueArray       *value_array,
                                            const GValue         *value);
LigmaValueArray * ligma_value_array_append   (LigmaValueArray       *value_array,
                                            const GValue         *value);
LigmaValueArray * ligma_value_array_insert   (LigmaValueArray       *value_array,
                                            gint                  index,
                                            const GValue         *value);

LigmaValueArray * ligma_value_array_remove   (LigmaValueArray       *value_array,
                                            gint                  index);
void             ligma_value_array_truncate (LigmaValueArray       *value_array,
                                            gint                  n_values);


/*
 * LIGMA_TYPE_PARAM_VALUE_ARRAY
 */

#define LIGMA_TYPE_PARAM_VALUE_ARRAY           (ligma_param_value_array_get_type ())
#define LIGMA_IS_PARAM_SPEC_VALUE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_VALUE_ARRAY))
#define LIGMA_PARAM_SPEC_VALUE_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_VALUE_ARRAY, LigmaParamSpecValueArray))

typedef struct _LigmaParamSpecValueArray LigmaParamSpecValueArray;

/**
 * LigmaParamSpecValueArray:
 * @parent_instance:  private #GParamSpec portion
 * @element_spec:     the #GParamSpec of the array elements
 * @fixed_n_elements: default length of the array
 *
 * A #GParamSpec derived structure that contains the meta data for
 * value array properties.
 **/
struct _LigmaParamSpecValueArray
{
  GParamSpec  parent_instance;
  GParamSpec *element_spec;
  gint        fixed_n_elements;
};

GType        ligma_param_value_array_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_value_array     (const gchar    *name,
                                              const gchar    *nick,
                                              const gchar    *blurb,
                                              GParamSpec     *element_spec,
                                              GParamFlags     flags);


G_END_DECLS

#endif /* __LIGMA_VALUE_ARRAY_H__ */
