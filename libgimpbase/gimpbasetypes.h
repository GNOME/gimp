/* LIBLIGMA - The LIGMA Library
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

#ifndef __LIGMA_BASE_TYPES_H__
#define __LIGMA_BASE_TYPES_H__


#include <libligmacolor/ligmacolortypes.h>
#include <libligmamath/ligmamathtypes.h>

#include <libligmabase/ligmabaseenums.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* XXX FIXME move these to a separate file */

#ifdef LIGMA_DISABLE_DEPRECATION_WARNINGS
#define LIGMA_DEPRECATED
#define LIGMA_DEPRECATED_FOR(f)
#define LIGMA_UNAVAILABLE(maj,min)
#else
#define LIGMA_DEPRECATED G_DEPRECATED
#define LIGMA_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f)
#define LIGMA_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min)
#endif


typedef struct _LigmaParasite     LigmaParasite;
typedef struct _LigmaEnumDesc     LigmaEnumDesc;
typedef struct _LigmaFlagsDesc    LigmaFlagsDesc;
typedef struct _LigmaValueArray   LigmaValueArray;

typedef struct _LigmaMetadata     LigmaMetadata;


/**
 * LigmaEnumDesc:
 * @value:      An enum value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for enum values. See ligma_enum_set_value_descriptions().
 **/
struct _LigmaEnumDesc
{
  gint         value;
  const gchar *value_desc;
  const gchar *value_help;
};

/**
 * LigmaFlagsDesc:
 * @value:      A flag value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for flag values. See ligma_flags_set_value_descriptions().
 **/
struct _LigmaFlagsDesc
{
  guint        value;
  const gchar *value_desc;
  const gchar *value_help;
};


void                  ligma_type_set_translation_domain  (GType                type,
                                                         const gchar         *domain);
const gchar         * ligma_type_get_translation_domain  (GType                type);

void                  ligma_type_set_translation_context (GType                type,
                                                         const gchar         *context);
const gchar         * ligma_type_get_translation_context (GType                type);

void                  ligma_enum_set_value_descriptions  (GType                enum_type,
                                                         const LigmaEnumDesc  *descriptions);
const LigmaEnumDesc  * ligma_enum_get_value_descriptions  (GType                enum_type);

void                  ligma_flags_set_value_descriptions (GType                flags_type,
                                                         const LigmaFlagsDesc *descriptions);
const LigmaFlagsDesc * ligma_flags_get_value_descriptions (GType                flags_type);


G_END_DECLS

#endif  /* __LIGMA_BASE_TYPES_H__ */
