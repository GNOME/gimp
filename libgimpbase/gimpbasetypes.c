/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpbasetypes.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "gimpbasetypes.h"


static GQuark  gimp_translation_domain_quark (void) G_GNUC_CONST;


/**
 * gimp_type_set_translation_domain:
 * @type:   a #GType
 * @domain: a constant string that identifies a translation domain or %NULL
 *
 * This function attaches a constant string as a gettext translation
 * domain identifier to a #GType. The only purpose of this function is
 * to use it when registering a #GTypeEnum with translatable value
 * names.
 *
 * Since: GIMP 2.2
 **/
void
gimp_type_set_translation_domain (GType        type,
                                  const gchar *domain)
{
  g_type_set_qdata (type,
                    gimp_translation_domain_quark (), (gpointer) domain);
}

/**
 * gimp_type_get_translation_domain:
 * @type: a #GType
 *
 * Retrieves the gettext translation domain identifier that has been
 * previously set using gimp_type_set_translation_domain(). You should
 * not need to use this function directly, use gimp_enum_get_value()
 * or gimp_enum_value_get_name() instead.
 *
 * Return value: the translation domain associated with @type
 *               or %NULL if no domain was set
 *
 * Since: GIMP 2.2
 **/
const gchar *
gimp_type_get_translation_domain (GType type)
{
  return (const gchar *) g_type_get_qdata (type,
                                           gimp_translation_domain_quark ());
}

static GQuark
gimp_translation_domain_quark (void)
{
  static GQuark quark = 0;

  if (! quark)
    quark = g_quark_from_static_string ("gimp-translation-domain-quark");

  return quark;
}
