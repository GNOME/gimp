/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __GIMP_BASE_TYPES_H__
#define __GIMP_BASE_TYPES_H__


#include <libgimpcolor/gimpcolortypes.h>
#include <libgimpmath/gimpmathtypes.h>

#include <libgimpbase/gimpbaseenums.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* XXX FIXME move these to a separate file */

#ifdef GIMP_DISABLE_DEPRECATION_WARNINGS
#define GIMP_DEPRECATED
#define GIMP_DEPRECATED_FOR(f)
#define GIMP_UNAVAILABLE(maj,min)
#else
#define GIMP_DEPRECATED G_DEPRECATED
#define GIMP_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f)
#define GIMP_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min)
#endif


typedef struct _GimpChoice        GimpChoice;
typedef struct _GimpParasite      GimpParasite;
typedef struct _GimpEnumDesc      GimpEnumDesc;
typedef struct _GimpExportOptions GimpExportOptions;
typedef struct _GimpFlagsDesc     GimpFlagsDesc;
typedef struct _GimpUnit          GimpUnit;
typedef struct _GimpValueArray    GimpValueArray;

typedef struct _GimpMetadata      GimpMetadata;


/**
 * GimpEnumDesc:
 * @value:      An enum value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for enum values. See gimp_enum_set_value_descriptions().
 **/
struct _GimpEnumDesc
{
  gint         value;
  const gchar *value_desc;
  const gchar *value_help;
};

/**
 * GimpFlagsDesc:
 * @value:      A flag value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for flag values. See gimp_flags_set_value_descriptions().
 **/
struct _GimpFlagsDesc
{
  guint        value;
  const gchar *value_desc;
  const gchar *value_help;
};


void                  gimp_type_set_translation_domain  (GType                type,
                                                         const gchar         *domain);
const gchar         * gimp_type_get_translation_domain  (GType                type);

void                  gimp_type_set_translation_context (GType                type,
                                                         const gchar         *context);
const gchar         * gimp_type_get_translation_context (GType                type);

void                  gimp_enum_set_value_descriptions  (GType                enum_type,
                                                         const GimpEnumDesc  *descriptions);
const GimpEnumDesc  * gimp_enum_get_value_descriptions  (GType                enum_type);

void                  gimp_flags_set_value_descriptions (GType                flags_type,
                                                         const GimpFlagsDesc *descriptions);
const GimpFlagsDesc * gimp_flags_get_value_descriptions (GType                flags_type);


G_END_DECLS

#endif  /* __GIMP_BASE_TYPES_H__ */
