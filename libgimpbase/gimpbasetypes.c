/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmabasetypes.c
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#include "config.h"

#include <glib-object.h>

#include "ligmabasetypes.h"


/**
 * SECTION: ligmabasetypes
 * @title: ligmabasetypes
 * @short_description: Translation between gettext translation domain
 *                     identifier and GType.
 *
 * Translation between gettext translation domain identifier and
 * GType.
 **/


static GQuark  ligma_translation_domain_quark  (void) G_GNUC_CONST;
static GQuark  ligma_translation_context_quark (void) G_GNUC_CONST;
static GQuark  ligma_value_descriptions_quark  (void) G_GNUC_CONST;


/**
 * ligma_type_set_translation_domain:
 * @type:   a #GType
 * @domain: a constant string that identifies a translation domain or %NULL
 *
 * This function attaches a constant string as a gettext translation
 * domain identifier to a #GType. The only purpose of this function is
 * to use it when registering a #G_TYPE_ENUM with translatable value
 * names.
 *
 * Since: 2.2
 **/
void
ligma_type_set_translation_domain (GType        type,
                                  const gchar *domain)
{
  g_type_set_qdata (type,
                    ligma_translation_domain_quark (), (gpointer) domain);
}

/**
 * ligma_type_get_translation_domain:
 * @type: a #GType
 *
 * Retrieves the gettext translation domain identifier that has been
 * previously set using ligma_type_set_translation_domain(). You should
 * not need to use this function directly, use ligma_enum_get_value()
 * or ligma_enum_value_get_desc() instead.
 *
 * Returns: the translation domain associated with @type
 *               or %NULL if no domain was set
 *
 * Since: 2.2
 **/
const gchar *
ligma_type_get_translation_domain (GType type)
{
  return (const gchar *) g_type_get_qdata (type,
                                           ligma_translation_domain_quark ());
}

/**
 * ligma_type_set_translation_context:
 * @type:    a #GType
 * @context: a constant string that identifies a translation context or %NULL
 *
 * This function attaches a constant string as a translation context
 * to a #GType. The only purpose of this function is to use it when
 * registering a #G_TYPE_ENUM with translatable value names.
 *
 * Since: 2.8
 **/
void
ligma_type_set_translation_context (GType        type,
                                   const gchar *context)
{
  g_type_set_qdata (type,
                    ligma_translation_context_quark (), (gpointer) context);
}

/**
 * ligma_type_get_translation_context:
 * @type: a #GType
 *
 * Retrieves the translation context that has been previously set
 * using ligma_type_set_translation_context(). You should not need to
 * use this function directly, use ligma_enum_get_value() or
 * ligma_enum_value_get_desc() instead.
 *
 * Returns: the translation context associated with @type
 *               or %NULL if no context was set
 *
 * Since: 2.8
 **/
const gchar *
ligma_type_get_translation_context (GType type)
{
  return (const gchar *) g_type_get_qdata (type,
                                           ligma_translation_context_quark ());
}

/**
 * ligma_enum_set_value_descriptions:
 * @enum_type:    a #GType
 * @descriptions: a %NULL terminated constant static array of #LigmaEnumDesc
 *
 * Sets the array of human readable and translatable descriptions
 * and help texts for enum values.
 *
 * Since: 2.2
 **/
void
ligma_enum_set_value_descriptions (GType               enum_type,
                                  const LigmaEnumDesc *descriptions)
{
  g_return_if_fail (g_type_is_a (enum_type, G_TYPE_ENUM));
  g_return_if_fail (descriptions != NULL);

  g_type_set_qdata (enum_type,
                    ligma_value_descriptions_quark (),
                    (gpointer) descriptions);
}

/**
 * ligma_enum_get_value_descriptions:
 * @enum_type: a #GType
 *
 * Retrieves the array of human readable and translatable descriptions
 * and help texts for enum values.
 *
 * Returns: a %NULL terminated constant array of #LigmaEnumDesc
 *
 * Since: 2.2
 **/
const LigmaEnumDesc *
ligma_enum_get_value_descriptions (GType enum_type)
{
  g_return_val_if_fail (g_type_is_a (enum_type, G_TYPE_ENUM), NULL);

  return (const LigmaEnumDesc *)
    g_type_get_qdata (enum_type, ligma_value_descriptions_quark ());
}

/**
 * ligma_flags_set_value_descriptions:
 * @flags_type:   a #GType
 * @descriptions: a %NULL terminated constant static array of #LigmaFlagsDesc
 *
 * Sets the array of human readable and translatable descriptions
 * and help texts for flags values.
 *
 * Since: 2.2
 **/
void
ligma_flags_set_value_descriptions (GType                flags_type,
                                   const LigmaFlagsDesc *descriptions)
{
  g_return_if_fail (g_type_is_a (flags_type, G_TYPE_FLAGS));
  g_return_if_fail (descriptions != NULL);

  g_type_set_qdata (flags_type,
                    ligma_value_descriptions_quark (),
                    (gpointer) descriptions);
}

/**
 * ligma_flags_get_value_descriptions:
 * @flags_type: a #GType
 *
 * Retrieves the array of human readable and translatable descriptions
 * and help texts for flags values.
 *
 * Returns: a %NULL terminated constant array of #LigmaFlagsDesc
 *
 * Since: 2.2
 **/
const LigmaFlagsDesc *
ligma_flags_get_value_descriptions (GType flags_type)
{
  g_return_val_if_fail (g_type_is_a (flags_type, G_TYPE_FLAGS), NULL);

  return (const LigmaFlagsDesc *)
    g_type_get_qdata (flags_type, ligma_value_descriptions_quark ());
}


/*  private functions  */

static GQuark
ligma_translation_domain_quark (void)
{
  static GQuark quark = 0;

  if (! quark)
    quark = g_quark_from_static_string ("ligma-translation-domain-quark");

  return quark;
}

static GQuark
ligma_translation_context_quark (void)
{
  static GQuark quark = 0;

  if (! quark)
    quark = g_quark_from_static_string ("ligma-translation-context-quark");

  return quark;
}

static GQuark
ligma_value_descriptions_quark (void)
{
  static GQuark quark = 0;

  if (! quark)
    quark = g_quark_from_static_string ("ligma-value-descriptions-quark");

  return quark;
}
